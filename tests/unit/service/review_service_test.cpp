#include "service/review_service.h"

#include <gtest/gtest.h>

#include <ctime>

#include "db/card_dao.h"
#include "db/database.h"
#include "db/migrations.h"
#include "db/review_dao.h"
#include "db/topic_dao.h"

namespace bagu::service {

class ReviewServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto r = db::Database::open(":memory:");
        ASSERT_TRUE(r.is_ok());
        db_ = std::make_unique<db::Database>(std::move(r.value()));
        db::register_all_migrations(*db_);
        ASSERT_TRUE(db_->migrate().is_ok());
        svc_ = std::make_unique<ReviewService>(*db_);

        // 一个 topic + 3 张 card
        db::TopicDao tdao(*db_);
        db::Topic t;
        t.name = "mysql"; t.title = "MySQL"; t.file_path = "/p"; t.file_hash = "h";
        t.imported_at = 0; t.updated_at = 0;
        topic_id_ = tdao.insert(t).value();

        db::CardDao cdao(*db_);
        for (int i = 1; i <= 3; ++i) {
            db::Card c;
            c.topic_id = topic_id_;
            c.question = "Q" + std::to_string(i);
            c.answer = "A" + std::to_string(i);
            c.created_at = 0; c.updated_at = 0;
            card_ids_.push_back(cdao.insert(c).value());
        }
    }

    std::unique_ptr<db::Database> db_;
    std::unique_ptr<ReviewService> svc_;
    int64_t topic_id_ = 0;
    std::vector<int64_t> card_ids_;
};

TEST_F(ReviewServiceTest, GetDueCards_FreshDb_ReturnsOnlyNew) {
    ReviewPlan plan;
    plan.topic_id = topic_id_;
    plan.max_due = 100;
    plan.max_new = 5;
    auto r = svc_->get_due_cards(plan);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().size(), 3u);
    for (const auto& d : r.value()) {
        EXPECT_TRUE(d.is_new);
    }
}

TEST_F(ReviewServiceTest, SubmitReview_SetsLastAndNextReview) {
    auto r = svc_->submit_review(card_ids_[0], 5, 1500);
    ASSERT_TRUE(r.is_ok());
    auto& row = r.value();
    EXPECT_EQ(row.review_count, 1);
    EXPECT_EQ(row.correct_count, 1);
    EXPECT_EQ(row.repetitions, 1);
    EXPECT_EQ(row.interval_days, 1);
    EXPECT_GT(row.next_review, row.last_review);
}

TEST_F(ReviewServiceTest, SubmitReview_FailedScore_ResetsRep) {
    // 先答对一次
    svc_->submit_review(card_ids_[0], 5, 0);
    svc_->submit_review(card_ids_[0], 5, 0);

    // 再答错
    auto r = svc_->submit_review(card_ids_[0], 0, 0);
    ASSERT_TRUE(r.is_ok());
    auto& row = r.value();
    EXPECT_EQ(row.repetitions, 0);
    EXPECT_EQ(row.interval_days, 1);
    EXPECT_EQ(row.review_count, 3);
    EXPECT_EQ(row.correct_count, 2);   // 第三次没答对
}

TEST_F(ReviewServiceTest, GetDueCards_AfterReview_LimitsByDate) {
    // 学一次 → next_review 在 1 天后
    svc_->submit_review(card_ids_[0], 5, 0);

    ReviewPlan plan;
    plan.topic_id = topic_id_;
    plan.max_due = 100;
    plan.max_new = 0;  // 不取新卡
    auto r = svc_->get_due_cards(plan);
    ASSERT_TRUE(r.is_ok());
    // 已学的下次是 1 天后，今天不到期；其他两张是新卡但 max_new=0
    EXPECT_EQ(r.value().size(), 0u);
}

TEST_F(ReviewServiceTest, History_RecordedOnEachSubmit) {
    svc_->submit_review(card_ids_[0], 5, 100);
    svc_->submit_review(card_ids_[0], 4, 200);
    svc_->submit_review(card_ids_[1], 3, 300);

    auto stmt = db_->prepare("SELECT COUNT(*) FROM review_history");
    ASSERT_TRUE(stmt.step());
    EXPECT_EQ(stmt.column_int(0), 3);
}

TEST_F(ReviewServiceTest, GetDueCards_RespectsTopicFilter) {
    // 加另一个 topic 的卡片
    db::TopicDao tdao(*db_);
    db::Topic t2;
    t2.name = "redis"; t2.title = "R"; t2.file_path = "/p"; t2.file_hash = "h2";
    t2.imported_at = 0; t2.updated_at = 0;
    int64_t other_id = tdao.insert(t2).value();

    db::CardDao cdao(*db_);
    db::Card c;
    c.topic_id = other_id;
    c.question = "redis Q"; c.answer = "A";
    c.created_at = 0; c.updated_at = 0;
    cdao.insert(c);

    ReviewPlan plan;
    plan.topic_id = topic_id_;
    plan.max_new = 10;
    auto r = svc_->get_due_cards(plan);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().size(), 3u);  // 仅 mysql 的 3 张
    for (const auto& d : r.value()) {
        EXPECT_EQ(d.topic_id, topic_id_);
    }
}

// ===== due_summary =====

TEST_F(ReviewServiceTest, DueSummary_AllNewWhenNoReviews) {
    auto r = svc_->due_summary();
    ASSERT_TRUE(r.is_ok());
    auto& s = r.value();
    EXPECT_EQ(s.total_due, 0);
    EXPECT_EQ(s.total_new, 3);   // SetUp 创建 3 张 mysql 卡
    ASSERT_EQ(s.per_topic.size(), 1u);
    EXPECT_EQ(s.per_topic[0].topic_name, "mysql");
    EXPECT_EQ(s.per_topic[0].new_cards, 3);
    EXPECT_EQ(s.per_topic[0].due, 0);
}

TEST_F(ReviewServiceTest, DueSummary_AfterReview_CountsDue) {
    // 第 1 张：复习 5 分（隔 1 天）→ 不到期
    svc_->submit_review(card_ids_[0], 5, 0);
    // 第 2 张：复习 + 手工把 next_review 改成过去 → 到期
    svc_->submit_review(card_ids_[1], 4, 0);
    {
        auto u = db_->prepare("UPDATE review SET next_review = 1 WHERE card_id = ?");
        u.bind(1, card_ids_[1]);
        ASSERT_GE(u.execute(), 0);
    }
    // 第 3 张：保持新卡

    auto r = svc_->due_summary();
    ASSERT_TRUE(r.is_ok());
    auto& s = r.value();
    EXPECT_EQ(s.total_due, 1);
    EXPECT_EQ(s.total_new, 1);
}

TEST_F(ReviewServiceTest, DueSummary_SkipsTopicsWithNoCards) {
    db::TopicDao tdao(*db_);
    db::Topic t;
    t.name = "empty"; t.title = "E"; t.file_path = "/p"; t.file_hash = "h3";
    t.imported_at = 0; t.updated_at = 0;
    tdao.insert(t);

    auto r = svc_->due_summary();
    ASSERT_TRUE(r.is_ok());
    for (const auto& tp : r.value().per_topic) {
        EXPECT_NE(tp.topic_name, "empty");
    }
}

TEST_F(ReviewServiceTest, DueSummary_RespectsSuspendedFlag) {
    svc_->submit_review(card_ids_[0], 4, 0);
    auto u = db_->prepare(
        "UPDATE review SET next_review = 1, suspended = 1 WHERE card_id = ?");
    u.bind(1, card_ids_[0]);
    ASSERT_GE(u.execute(), 0);

    auto r = svc_->due_summary();
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().total_due, 0);
}

}  // namespace bagu::service
