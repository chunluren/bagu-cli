#include "db/review_dao.h"

namespace bagu::db {

namespace {

ReviewRow parse_review_row(Statement& s) {
    ReviewRow r;
    r.card_id       = s.column_int64(0);
    r.last_review   = s.column_int64(1);
    r.next_review   = s.column_int64(2);
    r.interval_days = s.column_int(3);
    r.ease_factor   = s.column_double(4);
    r.repetitions   = s.column_int(5);
    r.review_count  = s.column_int(6);
    r.correct_count = s.column_int(7);
    r.suspended     = s.column_int(8) != 0;
    return r;
}

}  // namespace

Result<std::optional<ReviewRow>> ReviewDao::find(int64_t card_id) {
    auto stmt = db_.prepare(
        "SELECT card_id, last_review, next_review, interval_days, "
        "ease_factor, repetitions, review_count, correct_count, suspended "
        "FROM review WHERE card_id = ?");
    if (!stmt) return make_err<std::optional<ReviewRow>>(E::kDbPrepareFailed,
                                                         "prepare failed");
    stmt.bind(1, card_id);
    if (!stmt.step()) return std::optional<ReviewRow>(std::nullopt);
    return std::optional<ReviewRow>(parse_review_row(stmt));
}

Result<void> ReviewDao::upsert(const ReviewRow& r) {
    // SQLite UPSERT
    auto stmt = db_.prepare(
        "INSERT INTO review "
        "(card_id, last_review, next_review, interval_days, ease_factor, "
        " repetitions, review_count, correct_count, suspended) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?) "
        "ON CONFLICT(card_id) DO UPDATE SET "
        "last_review=excluded.last_review, "
        "next_review=excluded.next_review, "
        "interval_days=excluded.interval_days, "
        "ease_factor=excluded.ease_factor, "
        "repetitions=excluded.repetitions, "
        "review_count=excluded.review_count, "
        "correct_count=excluded.correct_count, "
        "suspended=excluded.suspended");
    if (!stmt) return make_err(E::kDbPrepareFailed, "prepare upsert failed");

    stmt.bind(1, r.card_id);
    stmt.bind(2, r.last_review);
    stmt.bind(3, r.next_review);
    stmt.bind(4, r.interval_days);
    stmt.bind(5, r.ease_factor);
    stmt.bind(6, r.repetitions);
    stmt.bind(7, r.review_count);
    stmt.bind(8, r.correct_count);
    stmt.bind(9, r.suspended ? 1 : 0);

    if (stmt.execute() < 0) return make_err(E::kDbExecuteFailed, "upsert failed");
    return Result<void>::ok();
}

Result<void> ReviewDao::add_history(int64_t card_id, int score,
                                    int duration_ms, int64_t reviewed_at) {
    auto stmt = db_.prepare(
        "INSERT INTO review_history (card_id, reviewed_at, score, duration_ms) "
        "VALUES (?, ?, ?, ?)");
    if (!stmt) return make_err(E::kDbPrepareFailed, "prepare history failed");
    stmt.bind(1, card_id);
    stmt.bind(2, reviewed_at);
    stmt.bind(3, score);
    stmt.bind(4, duration_ms);
    if (stmt.execute() < 0) return make_err(E::kDbExecuteFailed, "insert history failed");
    return Result<void>::ok();
}

Result<std::vector<DueCard>> ReviewDao::find_due_cards(int64_t now_ts,
                                                      int64_t topic_id,
                                                      int max_due,
                                                      int max_new) {
    std::vector<DueCard> out;

    // ===== 1. 已到期的（review.next_review <= now）=====
    {
        std::string sql =
            "SELECT card.id, card.topic_id, topic.name, "
            "       card.question, card.answer, card.card_type, "
            "       review.last_review, review.next_review, "
            "       review.interval_days, review.ease_factor, "
            "       review.repetitions, review.review_count, "
            "       review.correct_count, review.suspended "
            "FROM card "
            "JOIN review ON review.card_id = card.id "
            "JOIN topic ON topic.id = card.topic_id "
            "WHERE review.next_review <= ? AND review.suspended = 0";
        if (topic_id > 0) sql += " AND card.topic_id = ?";
        sql += " ORDER BY review.next_review ASC LIMIT ?";

        auto stmt = db_.prepare(sql);
        if (!stmt) return make_err<std::vector<DueCard>>(E::kDbPrepareFailed,
                                                          "prepare due failed");
        int idx = 1;
        stmt.bind(idx++, now_ts);
        if (topic_id > 0) stmt.bind(idx++, topic_id);
        stmt.bind(idx, max_due);

        while (stmt.step()) {
            DueCard d;
            d.card_id     = stmt.column_int64(0);
            d.topic_id    = stmt.column_int64(1);
            d.topic_name  = stmt.column_text(2);
            d.question    = stmt.column_text(3);
            d.answer      = stmt.column_text(4);
            d.card_type   = stmt.column_text(5);
            d.is_new = false;
            d.state.card_id      = d.card_id;
            d.state.last_review   = stmt.column_int64(6);
            d.state.next_review   = stmt.column_int64(7);
            d.state.interval_days = stmt.column_int(8);
            d.state.ease_factor   = stmt.column_double(9);
            d.state.repetitions   = stmt.column_int(10);
            d.state.review_count  = stmt.column_int(11);
            d.state.correct_count = stmt.column_int(12);
            d.state.suspended     = stmt.column_int(13) != 0;
            out.push_back(std::move(d));
        }
    }

    // ===== 2. 新卡片（没有 review 记录）=====
    if (max_new > 0) {
        std::string sql =
            "SELECT card.id, card.topic_id, topic.name, "
            "       card.question, card.answer, card.card_type "
            "FROM card "
            "JOIN topic ON topic.id = card.topic_id "
            "LEFT JOIN review ON review.card_id = card.id "
            "WHERE review.card_id IS NULL";
        if (topic_id > 0) sql += " AND card.topic_id = ?";
        sql += " ORDER BY card.id ASC LIMIT ?";

        auto stmt = db_.prepare(sql);
        if (!stmt) return make_err<std::vector<DueCard>>(E::kDbPrepareFailed,
                                                          "prepare new failed");
        int idx = 1;
        if (topic_id > 0) stmt.bind(idx++, topic_id);
        stmt.bind(idx, max_new);

        while (stmt.step()) {
            DueCard d;
            d.card_id    = stmt.column_int64(0);
            d.topic_id   = stmt.column_int64(1);
            d.topic_name = stmt.column_text(2);
            d.question   = stmt.column_text(3);
            d.answer     = stmt.column_text(4);
            d.card_type  = stmt.column_text(5);
            d.is_new = true;
            d.state.card_id = d.card_id;
            out.push_back(std::move(d));
        }
    }

    return out;
}

Result<int> ReviewDao::count_due(int64_t now_ts, int64_t topic_id) {
    std::string sql =
        "SELECT COUNT(*) FROM review "
        "WHERE next_review <= ? AND suspended = 0";
    if (topic_id > 0) {
        sql += " AND card_id IN (SELECT id FROM card WHERE topic_id = ?)";
    }
    auto stmt = db_.prepare(sql);
    if (!stmt) return make_err<int>(E::kDbPrepareFailed, "prepare count failed");

    int idx = 1;
    stmt.bind(idx++, now_ts);
    if (topic_id > 0) stmt.bind(idx, topic_id);
    if (!stmt.step()) return 0;
    return stmt.column_int(0);
}

Result<int> ReviewDao::count_new(int64_t topic_id) {
    std::string sql =
        "SELECT COUNT(*) FROM card "
        "LEFT JOIN review ON review.card_id = card.id "
        "WHERE review.card_id IS NULL";
    if (topic_id > 0) sql += " AND card.topic_id = ?";

    auto stmt = db_.prepare(sql);
    if (!stmt) return make_err<int>(E::kDbPrepareFailed, "prepare count failed");
    if (topic_id > 0) stmt.bind(1, topic_id);
    if (!stmt.step()) return 0;
    return stmt.column_int(0);
}

Result<void> ReviewDao::set_suspended(int64_t card_id, bool suspended) {
    {
        auto ins = db_.prepare(
            "INSERT OR IGNORE INTO review (card_id) VALUES (?)");
        if (!ins) return make_err(E::kDbPrepareFailed, "prepare insert failed");
        ins.bind(1, card_id);
        if (ins.execute() < 0) {
            return make_err(E::kDbExecuteFailed, "insert review failed");
        }
    }
    auto upd = db_.prepare(
        "UPDATE review SET suspended = ? WHERE card_id = ?");
    if (!upd) return make_err(E::kDbPrepareFailed, "prepare update failed");
    upd.bind(1, suspended ? 1 : 0);
    upd.bind(2, card_id);
    if (upd.execute() < 0) {
        return make_err(E::kDbExecuteFailed, "update suspended failed");
    }
    return Result<void>::ok();
}

Result<int> ReviewDao::set_suspended_by_topic(int64_t topic_id, bool suspended) {
    {
        auto ins = db_.prepare(
            "INSERT OR IGNORE INTO review (card_id) "
            "SELECT id FROM card WHERE topic_id = ?");
        if (!ins) return make_err<int>(E::kDbPrepareFailed,
            "prepare backfill failed");
        ins.bind(1, topic_id);
        if (ins.execute() < 0) {
            return make_err<int>(E::kDbExecuteFailed, "backfill failed");
        }
    }
    auto upd = db_.prepare(
        "UPDATE review SET suspended = ? "
        "WHERE card_id IN (SELECT id FROM card WHERE topic_id = ?)");
    if (!upd) return make_err<int>(E::kDbPrepareFailed, "prepare update failed");
    upd.bind(1, suspended ? 1 : 0);
    upd.bind(2, topic_id);
    int affected = upd.execute();
    if (affected < 0) return make_err<int>(E::kDbExecuteFailed, "update failed");
    return affected;
}

}  // namespace bagu::db
