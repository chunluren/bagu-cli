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

private:
    db::Database& db_;
};

}  // namespace bagu::service
