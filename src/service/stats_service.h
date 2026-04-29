#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "bagu/result.h"
#include "db/database.h"

namespace bagu::service {

/// 单主题进度
struct TopicProgress {
    int64_t topic_id = 0;
    std::string topic_name;
    std::string title;
    int total_cards = 0;
    int learned_cards = 0;          // 至少复习过一次的卡片数
    int correct_cards = 0;          // 上次复习评分 >= 3
    double accuracy = 0.0;           // 全历史正确率（score >= 3 / total）
    int due_today = 0;
};

/// 总览统计
struct OverallStats {
    int total_topics = 0;
    int total_cards = 0;
    int total_reviews = 0;          // review_history 行数
    int total_correct = 0;
    int learned_unique_cards = 0;
    double overall_accuracy = 0.0;
    int streak_days = 0;             // 连续学习天数（含今天）
    int active_days_30 = 0;          // 最近 30 天活跃天数
};

/// 一天的复习计数（热力图用）
struct DailyCount {
    std::string date;                // "YYYY-MM-DD"（本地时区）
    int count = 0;                    // 当日复习数
};

/// 薄弱卡片（最近多次答错）
struct WeakCard {
    int64_t card_id = 0;
    std::string topic_name;
    std::string question;
    int wrong_count = 0;             // 最近评分 < 3 的次数
    int total_recent = 0;             // 最近复习总次数
};

class StatsService {
public:
    explicit StatsService(db::Database& db) : db_(db) {}

    Result<OverallStats> overall();

    Result<std::vector<TopicProgress>> per_topic();

    /// 取每天的复习计数（最近 N 天）
    /// @return 长度为 N 的数组，从最早到今天
    Result<std::vector<DailyCount>> daily_counts(int days);

    /// 取最近 N 次复习中答错次数最多的卡片
    Result<std::vector<WeakCard>> weak_cards(int recent_history_n, int top_k);

private:
    db::Database& db_;
};

}  // namespace bagu::service
