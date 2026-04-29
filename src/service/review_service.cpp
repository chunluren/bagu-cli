#include "service/review_service.h"

#include <ctime>

#include "core/sm2.h"
#include "db/review_dao.h"

namespace bagu::service {

Result<std::vector<db::DueCard>>
ReviewService::get_due_cards(const ReviewPlan& plan) {
    db::ReviewDao dao(db_);
    int64_t now = static_cast<int64_t>(std::time(nullptr));
    return dao.find_due_cards(now, plan.topic_id, plan.max_due, plan.max_new);
}

Result<db::ReviewRow> ReviewService::submit_review(int64_t card_id, int score,
                                                  int duration_ms) {
    db::ReviewDao dao(db_);
    int64_t now = static_cast<int64_t>(std::time(nullptr));

    // 1. 取当前状态（不存在则用默认）
    auto cur_r = dao.find(card_id);
    if (cur_r.is_err()) {
        return Result<db::ReviewRow>(cur_r.error());
    }

    db::ReviewRow row;
    row.card_id = card_id;
    if (cur_r.value().has_value()) row = *cur_r.value();

    // 2. 调 SM-2 更新
    core::ReviewState state;
    state.interval_days = row.interval_days;
    state.ease_factor   = row.ease_factor;
    state.repetitions   = row.repetitions;

    auto next_state = core::SM2Algorithm::update(state, score);

    row.last_review   = now;
    row.next_review   = core::SM2Algorithm::next_review_time(next_state, now);
    row.interval_days = next_state.interval_days;
    row.ease_factor   = next_state.ease_factor;
    row.repetitions   = next_state.repetitions;
    row.review_count++;
    if (score >= 3) row.correct_count++;

    // 3. 事务：upsert review + 追加 history
    auto txn = db_.begin();

    auto u = dao.upsert(row);
    if (u.is_err()) return Result<db::ReviewRow>(u.error());

    auto h = dao.add_history(card_id, score, duration_ms, now);
    if (h.is_err()) return Result<db::ReviewRow>(h.error());

    auto cm = txn.commit();
    if (cm.is_err()) return Result<db::ReviewRow>(cm.error());

    return row;
}

}  // namespace bagu::service
