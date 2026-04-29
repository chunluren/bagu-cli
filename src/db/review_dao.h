#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "bagu/result.h"
#include "db/database.h"

namespace bagu::db {

/// review 表行（一个 card 一行）
struct ReviewRow {
    int64_t card_id = 0;
    int64_t last_review = 0;       // 0 = 未学过
    int64_t next_review = 0;
    int interval_days = 0;
    double ease_factor = 2.5;
    int repetitions = 0;
    int review_count = 0;
    int correct_count = 0;
    bool suspended = false;
};

/// review_history 行
struct ReviewHistoryRow {
    int64_t id = 0;
    int64_t card_id = 0;
    int64_t reviewed_at = 0;
    int score = 0;
    int duration_ms = 0;
};

/// 待复习的卡片视图（review join card 简化）
struct DueCard {
    int64_t card_id = 0;
    int64_t topic_id = 0;
    std::string topic_name;
    std::string question;
    std::string answer;
    std::string card_type;
    bool is_new = false;          // 没有 review 记录
    ReviewRow state;               // 若 is_new=true，state 为默认值
};

class ReviewDao {
public:
    explicit ReviewDao(Database& db) : db_(db) {}

    /// 根据 card_id 查 review 行；不存在返回空
    Result<std::optional<ReviewRow>> find(int64_t card_id);

    /// upsert：存在则 update，否则 insert
    Result<void> upsert(const ReviewRow& r);

    /// 追加一条历史记录
    Result<void> add_history(int64_t card_id, int score,
                            int duration_ms, int64_t reviewed_at);

    /// 查找已到期的卡片（含未学过的新卡）
    /// @param now_ts 当前时间
    /// @param topic_id 限定主题（0 不限）
    /// @param max_due 已到期的最大数（按 next_review 升序）
    /// @param max_new 新卡片的最大数
    Result<std::vector<DueCard>> find_due_cards(int64_t now_ts,
                                                int64_t topic_id,
                                                int max_due,
                                                int max_new);

    /// 统计今日待复习总数
    Result<int> count_due(int64_t now_ts, int64_t topic_id);

    /// 统计未学过的新卡数
    Result<int> count_new(int64_t topic_id);

private:
    Database& db_;
};

}  // namespace bagu::db
