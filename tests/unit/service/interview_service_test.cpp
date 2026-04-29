#include "service/interview_service.h"

#include <gtest/gtest.h>

#include "db/database.h"
#include "db/migrations.h"
#include "llm/mock_client.h"

namespace bagu::service {

class InterviewServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto r = db::Database::open(":memory:");
        ASSERT_TRUE(r.is_ok());
        db_ = std::make_unique<db::Database>(std::move(r.value()));
        db::register_all_migrations(*db_);
        ASSERT_TRUE(db_->migrate().is_ok());

        // 准备一个 topic 用于 sample_topic_cards
        db_->execute(
            "INSERT INTO topic (name, title, file_path, file_hash, imported_at, updated_at) "
            "VALUES ('mysql', 'MySQL', '/p', 'h', 0, 0)");

        auto mock = std::make_unique<llm::MockClient>();
        mock_ptr_ = mock.get();
        svc_ = std::make_unique<InterviewService>(*db_, std::move(mock));
    }

    std::unique_ptr<db::Database> db_;
    llm::MockClient* mock_ptr_;
    std::unique_ptr<InterviewService> svc_;
};

TEST_F(InterviewServiceTest, StartSession_CreatesRow) {
    InterviewOptions opts;
    opts.topic = "mysql";
    opts.question_count = 3;
    auto r = svc_->start_session(opts, "test-model");
    ASSERT_TRUE(r.is_ok());
    EXPECT_GT(r.value(), 0);

    auto session = db::InterviewDao(*db_).find_session(r.value());
    ASSERT_TRUE(session.value().has_value());
    EXPECT_EQ(session.value()->topic, "mysql");
    EXPECT_EQ(session.value()->llm_model, "test-model");
}

TEST_F(InterviewServiceTest, NextQuestion_UsesLlm) {
    InterviewOptions opts; opts.topic = "mysql";
    svc_->start_session(opts, "m");
    mock_ptr_->push_response("第一题：什么是 MVCC？");

    auto r = svc_->next_question(1, "", nullptr);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), "第一题：什么是 MVCC？");
}

TEST_F(InterviewServiceTest, GradeAnswer_ParsesScore) {
    InterviewOptions opts; opts.topic = "mysql";
    svc_->start_session(opts, "m");
    mock_ptr_->push_response(
        "评分：8/10\n"
        "✓ 答到了 MVCC 的三要素\n"
        "✗ 没说到 RC vs RR 的 Read View 时机差异\n"
        "💡 下次补充隔离级别细节");

    auto r = svc_->grade_answer("Q?", "我的回答", nullptr);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().score, 8);
    EXPECT_NE(r.value().feedback.find("MVCC"), std::string::npos);
}

TEST_F(InterviewServiceTest, ParseScore_VariousFormats) {
    EXPECT_EQ(InterviewService::parse_score("评分：7/10"), 7);
    EXPECT_EQ(InterviewService::parse_score("评分: 5/10"), 5);
    EXPECT_EQ(InterviewService::parse_score("评分 9/10"), 9);
    EXPECT_EQ(InterviewService::parse_score("Score: 6"), 6);
    EXPECT_EQ(InterviewService::parse_score("分数：4"), 4);
    EXPECT_EQ(InterviewService::parse_score("没有分数"), 0);
    EXPECT_EQ(InterviewService::parse_score("评分：15/10"), 10);    // clamp
    // "-3" 中的 - 不被正则匹配，所以会从 "3/10" 解析 → 3
    EXPECT_EQ(InterviewService::parse_score("评分：3/10"), 3);
}

TEST_F(InterviewServiceTest, FullFlow_SaveAndEnd) {
    InterviewOptions opts; opts.topic = "mysql";
    auto sid = svc_->start_session(opts, "m").value();

    // 出题
    mock_ptr_->push_response("Q1?");
    auto q = svc_->next_question(1, "", nullptr).value();

    // 评分
    mock_ptr_->push_response("评分：7/10\n✓ ok\n✗ x\n💡 y");
    auto g = svc_->grade_answer(q, "user answer", nullptr).value();

    // 保存
    auto sv = svc_->save_qa(sid, 1, q, "user answer", g, 1234);
    ASSERT_TRUE(sv.is_ok());

    // 结束
    auto e = svc_->end_session(sid, g.score, 1);
    ASSERT_TRUE(e.is_ok());

    auto session = db::InterviewDao(*db_).find_session(sid);
    EXPECT_EQ(session.value()->question_count, 1);
    EXPECT_DOUBLE_EQ(session.value()->total_score, 7.0);

    auto qa = db::InterviewDao(*db_).qa_list(sid);
    ASSERT_EQ(qa.value().size(), 1u);
    EXPECT_EQ(qa.value()[0].ai_score, 7);
}

}  // namespace bagu::service
