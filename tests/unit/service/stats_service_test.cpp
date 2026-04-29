#include "service/stats_service.h"

#include <gtest/gtest.h>

#include <ctime>

#include "db/database.h"
#include "db/migrations.h"

namespace bagu::service {

class StatsServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto r = db::Database::open(":memory:");
        ASSERT_TRUE(r.is_ok());
        db_ = std::make_unique<db::Database>(std::move(r.value()));
        db::register_all_migrations(*db_);
        ASSERT_TRUE(db_->migrate().is_ok());

        // 准备 1 个 topic + 3 张 card
        db_->execute(
            "INSERT INTO topic (name, title, file_path, file_hash, imported_at, updated_at) "
            "VALUES ('mysql', 'MySQL', '/p', 'h', 0, 0)");
        for (int i = 1; i <= 3; ++i) {
            db_->execute(
                "INSERT INTO card (topic_id, question, answer, created_at, updated_at) "
                "VALUES (1, 'Q" + std::to_string(i) + "', 'A', 0, 0)");
        }
        svc_ = std::make_unique<StatsService>(*db_);
    }

    int64_t now_ = std::time(nullptr);
    std::unique_ptr<db::Database> db_;
    std::unique_ptr<StatsService> svc_;

    void add_history(int card_id, int score, int64_t reviewed_at) {
        std::ostringstream ss;
        ss << "INSERT INTO review_history (card_id, reviewed_at, score, duration_ms) "
              "VALUES (" << card_id << ", " << reviewed_at << ", " << score << ", 1000)";
        db_->execute(ss.str());
    }
};

TEST_F(StatsServiceTest, EmptyDb_OverallReturnsZero) {
    auto r = svc_->overall();
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().total_reviews, 0);
    EXPECT_EQ(r.value().total_correct, 0);
    EXPECT_EQ(r.value().streak_days, 0);
    EXPECT_DOUBLE_EQ(r.value().overall_accuracy, 0.0);
    EXPECT_EQ(r.value().total_topics, 1);
    EXPECT_EQ(r.value().total_cards, 3);
}

TEST_F(StatsServiceTest, OverallStats_AfterReviews) {
    add_history(1, 5, now_ - 100);
    add_history(2, 3, now_ - 200);
    add_history(3, 1, now_ - 300);   // 答错

    auto r = svc_->overall();
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().total_reviews, 3);
    EXPECT_EQ(r.value().total_correct, 2);
    EXPECT_NEAR(r.value().overall_accuracy, 2.0/3.0, 0.001);
    EXPECT_EQ(r.value().learned_unique_cards, 3);
    EXPECT_GE(r.value().streak_days, 1);
}

TEST_F(StatsServiceTest, PerTopic_ReturnsLearnedCount) {
    add_history(1, 5, now_ - 100);
    add_history(2, 4, now_ - 200);

    auto r = svc_->per_topic();
    ASSERT_TRUE(r.is_ok());
    ASSERT_EQ(r.value().size(), 1u);
    EXPECT_EQ(r.value()[0].topic_name, "mysql");
    EXPECT_EQ(r.value()[0].total_cards, 3);
    EXPECT_EQ(r.value()[0].learned_cards, 2);
    EXPECT_GT(r.value()[0].accuracy, 0.0);
}

TEST_F(StatsServiceTest, DailyCounts_ReturnsRequestedDays) {
    add_history(1, 5, now_ - 100);
    add_history(2, 3, now_ - 86400 - 100);   // 昨天

    auto r = svc_->daily_counts(3);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().size(), 3u);
    int total = 0;
    for (const auto& d : r.value()) total += d.count;
    EXPECT_EQ(total, 2);
}

TEST_F(StatsServiceTest, WeakCards_ReturnsCardsWithErrors) {
    add_history(1, 5, now_ - 100);
    add_history(2, 1, now_ - 200);
    add_history(2, 0, now_ - 300);
    add_history(3, 4, now_ - 400);

    auto r = svc_->weak_cards(50, 5);
    ASSERT_TRUE(r.is_ok());
    ASSERT_GE(r.value().size(), 1u);
    EXPECT_EQ(r.value()[0].card_id, 2);   // 错最多
    EXPECT_EQ(r.value()[0].wrong_count, 2);
}

TEST_F(StatsServiceTest, StreakDays_CountsConsecutive) {
    // 在本地 midnight 后插入数据（确保跨过 streak 算法的天分界）
    std::time_t t = now_;
    std::tm lt{};
    localtime_r(&t, &lt);
    lt.tm_hour = 12; lt.tm_min = 0; lt.tm_sec = 0;
    int64_t today_noon = static_cast<int64_t>(std::mktime(&lt));

    add_history(1, 5, today_noon);                       // 今天
    add_history(2, 5, today_noon - 86400);               // 昨天
    add_history(3, 5, today_noon - 86400 * 2);            // 前天

    auto r = svc_->overall();
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().streak_days, 3);
}

}  // namespace bagu::service
