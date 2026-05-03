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

Result<DueSummary> ReviewService::due_summary() {
    DueSummary s;
    s.generated_at = static_cast<int64_t>(std::time(nullptr));

    // SQL：左外连接 review，按 topic 分组数 due / new
    // due:  next_review IS NOT NULL AND next_review <= now AND suspended = 0
    // new:  review row 不存在（card 还没复习过）
    // 注：c.id IS NOT NULL 关键 — LEFT JOIN 让"空 topic"产生 1 行 NULL card，
    // 不加这个条件会被算成 1 张新卡
    auto stmt = db_.prepare(
        "SELECT t.id, t.name, t.title, "
        "  SUM(CASE WHEN r.next_review IS NOT NULL "
        "        AND r.next_review <= ? "
        "        AND IFNULL(r.suspended, 0) = 0 "
        "      THEN 1 ELSE 0 END) AS due, "
        "  SUM(CASE WHEN c.id IS NOT NULL AND r.card_id IS NULL "
        "      THEN 1 ELSE 0 END) AS new_cnt "
        "FROM topic t "
        "LEFT JOIN card c  ON c.topic_id = t.id "
        "LEFT JOIN review r ON r.card_id  = c.id "
        "GROUP BY t.id, t.name, t.title "
        "ORDER BY t.name");
    if (!stmt) return make_err<DueSummary>(E::kDbPrepareFailed,
        "prepare due_summary failed");
    stmt.bind(1, s.generated_at);

    while (stmt.step()) {
        DueByTopic row;
        row.topic_id    = stmt.column_int64(0);
        row.topic_name  = stmt.column_text(1);
        row.topic_title = stmt.column_text(2);
        row.due         = stmt.column_int(3);
        row.new_cards   = stmt.column_int(4);
        // 跳过完全没卡片的 topic（如 readme.md → 0 cards）
        if (row.due == 0 && row.new_cards == 0) continue;
        s.total_due += row.due;
        s.total_new += row.new_cards;
        s.per_topic.push_back(std::move(row));
    }
    return s;
}

}  // namespace bagu::service
