#include "db/interview_dao.h"

#include <gtest/gtest.h>

#include "db/database.h"
#include "db/migrations.h"

namespace bagu::db {

class InterviewDaoTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto r = Database::open(":memory:");
        ASSERT_TRUE(r.is_ok());
        db_ = std::make_unique<Database>(std::move(r.value()));
        register_all_migrations(*db_);
        ASSERT_TRUE(db_->migrate().is_ok());
        dao_ = std::make_unique<InterviewDao>(*db_);
    }

    std::unique_ptr<Database> db_;
    std::unique_ptr<InterviewDao> dao_;
};

TEST_F(InterviewDaoTest, CreateSession_ReturnsId) {
    InterviewSession s;
    s.topic = "mysql";
    s.started_at = 1700000000;
    s.llm_provider = "openai";
    s.llm_model = "gpt-4o-mini";
    auto r = dao_->create_session(s);
    ASSERT_TRUE(r.is_ok());
    EXPECT_GT(r.value(), 0);
}

TEST_F(InterviewDaoTest, FindSession_AfterCreate_Returns) {
    InterviewSession s;
    s.topic = "redis";
    s.started_at = 1700000000;
    s.llm_provider = "openai";
    s.llm_model = "gpt-4o-mini";
    auto id = dao_->create_session(s).value();

    auto f = dao_->find_session(id);
    ASSERT_TRUE(f.is_ok());
    ASSERT_TRUE(f.value().has_value());
    EXPECT_EQ(f.value()->topic, "redis");
    EXPECT_EQ(f.value()->llm_provider, "openai");
    EXPECT_EQ(f.value()->ended_at, 0);    // 未 finalize
}

TEST_F(InterviewDaoTest, AddQA_AndList) {
    InterviewSession s;
    s.topic = "mysql"; s.started_at = 0;
    auto sid = dao_->create_session(s).value();

    InterviewQA qa1;
    qa1.session_id = sid; qa1.question_no = 1;
    qa1.question = "Q1?"; qa1.user_answer = "A1";
    qa1.ai_score = 7; qa1.ai_feedback = "good";
    qa1.duration_ms = 5000;
    dao_->add_qa(qa1);

    InterviewQA qa2 = qa1;
    qa2.question_no = 2; qa2.question = "Q2?"; qa2.user_answer = "A2";
    qa2.ai_score = 9;
    dao_->add_qa(qa2);

    auto list = dao_->qa_list(sid);
    ASSERT_TRUE(list.is_ok());
    ASSERT_EQ(list.value().size(), 2u);
    EXPECT_EQ(list.value()[0].question_no, 1);
    EXPECT_EQ(list.value()[0].ai_score, 7);
    EXPECT_EQ(list.value()[1].question_no, 2);
    EXPECT_EQ(list.value()[1].ai_score, 9);
}

TEST_F(InterviewDaoTest, FinalizeSession_UpdatesEndedAndScore) {
    InterviewSession s;
    s.topic = "x"; s.started_at = 1000;
    auto id = dao_->create_session(s).value();

    auto u = dao_->finalize_session(id, 2000, 7.5, 5);
    ASSERT_TRUE(u.is_ok());

    auto f = dao_->find_session(id);
    ASSERT_TRUE(f.value().has_value());
    EXPECT_EQ(f.value()->ended_at, 2000);
    EXPECT_DOUBLE_EQ(f.value()->total_score, 7.5);
    EXPECT_EQ(f.value()->question_count, 5);
}

TEST_F(InterviewDaoTest, RecentSessions_OrderByStartedDesc) {
    for (int i = 1; i <= 3; ++i) {
        InterviewSession s;
        s.topic = "t" + std::to_string(i);
        s.started_at = 1000 + i;
        dao_->create_session(s);
    }
    auto r = dao_->recent_sessions(10);
    ASSERT_TRUE(r.is_ok());
    ASSERT_EQ(r.value().size(), 3u);
    EXPECT_EQ(r.value()[0].topic, "t3");
    EXPECT_EQ(r.value()[2].topic, "t1");
}

}  // namespace bagu::db
