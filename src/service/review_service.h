#pragma once

#include <vector>

#include "bagu/result.h"
#include "db/database.h"
#include "db/review_dao.h"

namespace bagu::service {

struct ReviewPlan {
    int64_t topic_id = 0;        // 0 = 不限主题
    int max_due = 100;            // 已到期最多取多少
    int max_new = 5;              // 每天新学几张
};

/// 主题维度的到期摘要
struct DueByTopic {
    int64_t topic_id = 0;
    std::string topic_name;
    std::string topic_title;
    int due = 0;        // 已到期（next_review <= now）
    int new_cards = 0;   // 从未复习过
};

/// 全局到期摘要（首页 / `bagu due` 用）
struct DueSummary {
    int64_t generated_at = 0;       // unix ts
    int total_due = 0;
    int total_new = 0;
    std::vector<DueByTopic> per_topic;
};

/// 复习服务：编排 SM-2 算法 + ReviewDao
class ReviewService {
public:
    explicit ReviewService(db::Database& db) : db_(db) {}

    /// 取今日待复习的卡片（已到期 + 新卡片）
    Result<std::vector<db::DueCard>> get_due_cards(const ReviewPlan& plan);

    /// 提交一次评分 → 用 SM-2 更新 review 表 + 追加 history
    /// @param card_id  卡片 id
    /// @param score    0-5 评分
    /// @param duration_ms 答题耗时
    /// @return 更新后的 ReviewRow（包含 next_review 等供 UI 显示）
    Result<db::ReviewRow> submit_review(int64_t card_id, int score,
                                        int duration_ms);

    /// 全局到期摘要：每个 topic 的到期数 + 新卡数
    /// 用于首页 banner / `bagu due` / 桌面通知
    Result<DueSummary> due_summary();

private:
    db::Database& db_;
};

}  // namespace bagu::service
