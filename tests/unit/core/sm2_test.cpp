#include "core/sm2.h"

#include <gtest/gtest.h>

namespace bagu::core {

TEST(SM2, FreshCard_Score5_FirstIntervalIsOne) {
    ReviewState s{};
    auto next = SM2Algorithm::update(s, 5);
    EXPECT_EQ(next.interval_days, 1);
    EXPECT_EQ(next.repetitions, 1);
    EXPECT_GT(next.ease_factor, 2.5);
}

TEST(SM2, SecondCorrect_IntervalIsSix) {
    ReviewState s{.interval_days = 1, .ease_factor = 2.5, .repetitions = 1};
    auto next = SM2Algorithm::update(s, 4);
    EXPECT_EQ(next.interval_days, 6);
    EXPECT_EQ(next.repetitions, 2);
}

TEST(SM2, ThirdCorrect_IntervalGrowsByEase) {
    ReviewState s{.interval_days = 6, .ease_factor = 2.5, .repetitions = 2};
    auto next = SM2Algorithm::update(s, 4);
    // 6 * 2.5 = 15
    EXPECT_EQ(next.interval_days, 15);
    EXPECT_EQ(next.repetitions, 3);
}

TEST(SM2, Score0_ResetsRepetitionsAndIntervalToOne) {
    ReviewState s{.interval_days = 30, .ease_factor = 2.7, .repetitions = 5};
    auto next = SM2Algorithm::update(s, 0);
    EXPECT_EQ(next.interval_days, 1);
    EXPECT_EQ(next.repetitions, 0);
    EXPECT_LT(next.ease_factor, s.ease_factor);  // ease 减小
    EXPECT_GE(next.ease_factor, SM2Algorithm::min_ease_factor());
}

TEST(SM2, Score2_AlsoResets) {
    ReviewState s{.interval_days = 10, .ease_factor = 2.5, .repetitions = 3};
    auto next = SM2Algorithm::update(s, 2);
    EXPECT_EQ(next.interval_days, 1);
    EXPECT_EQ(next.repetitions, 0);
}

TEST(SM2, EaseFactor_NeverBelowMin) {
    ReviewState s{.interval_days = 1, .ease_factor = 1.3, .repetitions = 0};
    // 连续 5 次失败
    for (int i = 0; i < 5; ++i) {
        s = SM2Algorithm::update(s, 0);
        EXPECT_GE(s.ease_factor, SM2Algorithm::min_ease_factor());
    }
}

TEST(SM2, Score5_RaisesEaseFactor) {
    ReviewState s{.ease_factor = 2.5};
    auto next = SM2Algorithm::update(s, 5);
    EXPECT_GT(next.ease_factor, s.ease_factor);
}

TEST(SM2, Score3_KeepsEaseStableIsh) {
    // q=3: delta = 0.1 - 2*(0.08 + 2*0.02) = 0.1 - 0.24 = -0.14 → ease 略减
    ReviewState s{.ease_factor = 2.5};
    auto next = SM2Algorithm::update(s, 3);
    EXPECT_LT(next.ease_factor, s.ease_factor);
    EXPECT_GT(next.ease_factor, 2.0);
}

TEST(SM2, NextReviewTime_ProperlyOffsetsByDays) {
    ReviewState s{.interval_days = 7};
    int64_t now = 1700000000;
    int64_t next = SM2Algorithm::next_review_time(s, now);
    EXPECT_EQ(next - now, 7 * 24 * 60 * 60);
}

TEST(SM2, NextReviewTime_ZeroInterval) {
    ReviewState s{.interval_days = 0};
    EXPECT_EQ(SM2Algorithm::next_review_time(s, 100), 100);
}

TEST(SM2, ScoreClamped_OutOfRange) {
    ReviewState s{};
    auto a = SM2Algorithm::update(s, -1);
    auto b = SM2Algorithm::update(s, 0);
    EXPECT_EQ(a.repetitions, b.repetitions);
    EXPECT_EQ(a.interval_days, b.interval_days);

    auto c = SM2Algorithm::update(s, 100);
    auto d = SM2Algorithm::update(s, 5);
    EXPECT_EQ(c.repetitions, d.repetitions);
    EXPECT_EQ(c.interval_days, d.interval_days);
}

}  // namespace bagu::core
