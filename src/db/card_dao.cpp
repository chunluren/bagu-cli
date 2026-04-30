#include "db/card_dao.h"

#include <spdlog/spdlog.h>

namespace bagu::db {

namespace {

constexpr const char* kSelectCols =
    "id, topic_id, chapter_id, question, answer, "
    "difficulty, tags, source_line, card_type, created_at, updated_at";

}  // namespace

Card CardDao::parse_row(Statement& stmt) {
    Card c;
    c.id         = stmt.column_int64(0);
    c.topic_id   = stmt.column_int64(1);
    if (!stmt.is_null(2)) c.chapter_id = stmt.column_int64(2);
    c.question   = stmt.column_text(3);
    c.answer     = stmt.column_text(4);
    c.difficulty = stmt.column_int(5);
    c.tags       = stmt.column_text(6);
    c.source_line = stmt.column_int(7);
    c.card_type  = stmt.column_text(8);
    c.created_at = stmt.column_int64(9);
    c.updated_at = stmt.column_int64(10);
    return c;
}

Result<int64_t> CardDao::insert(const Card& c) {
    auto stmt = db_.prepare(
        "INSERT INTO card (topic_id, chapter_id, question, answer, "
        "difficulty, tags, source_line, card_type, created_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
    if (!stmt) return make_err<int64_t>(E::kDbPrepareFailed, "prepare failed");

    stmt.bind(1, c.topic_id);
    if (c.chapter_id.has_value()) {
        stmt.bind(2, *c.chapter_id);
    } else {
        stmt.bind(2, nullptr);
    }
    stmt.bind(3, c.question);
    stmt.bind(4, c.answer);
    stmt.bind(5, c.difficulty);
    stmt.bind(6, c.tags);
    stmt.bind(7, c.source_line);
    stmt.bind(8, c.card_type);
    stmt.bind(9, c.created_at);
    stmt.bind(10, c.updated_at);

    if (stmt.execute() < 0) {
        return make_err<int64_t>(E::kDbExecuteFailed, "insert card failed");
    }
    return db_.last_insert_rowid();
}

Result<int64_t> CardDao::insert_batch(const std::vector<Card>& cards) {
    int64_t inserted = 0;
    for (const auto& c : cards) {
        auto r = insert(c);
        if (r.is_err()) return r;
        inserted++;
    }
    return inserted;
}

Result<std::optional<Card>> CardDao::find_by_id(int64_t id) {
    auto stmt = db_.prepare(std::string("SELECT ") + kSelectCols
        + " FROM card WHERE id = ?");
    if (!stmt) return make_err<std::optional<Card>>(E::kDbPrepareFailed, "prepare failed");

    stmt.bind(1, id);
    if (!stmt.step()) return std::optional<Card>(std::nullopt);
    return std::optional<Card>(parse_row(stmt));
}

Result<std::vector<Card>> CardDao::find_by_topic(int64_t topic_id) {
    std::vector<Card> result;
    auto stmt = db_.prepare(std::string("SELECT ") + kSelectCols
        + " FROM card WHERE topic_id = ? ORDER BY id");
    if (!stmt) return make_err<std::vector<Card>>(E::kDbPrepareFailed, "prepare failed");

    stmt.bind(1, topic_id);
    while (stmt.step()) {
        result.push_back(parse_row(stmt));
    }
    return result;
}

Result<int64_t> CardDao::count_by_topic(int64_t topic_id) {
    auto stmt = db_.prepare("SELECT COUNT(*) FROM card WHERE topic_id = ?");
    if (!stmt) return make_err<int64_t>(E::kDbPrepareFailed, "prepare failed");
    stmt.bind(1, topic_id);
    if (!stmt.step()) return int64_t{0};
    return stmt.column_int64(0);
}

namespace {
/// 把用户的 keyword 转成 FTS5 安全的 phrase 查询：
///   "MVCC 实现" → "\"MVCC 实现\""
///   含特殊字符（OR / AND / NEAR / * / -）也都被当字面量
/// 内部双引号需转义为两个双引号（FTS5 语法）
std::string fts5_quote_phrase(const std::string& kw) {
    std::string out;
    out.reserve(kw.size() + 4);
    out.push_back('"');
    for (char c : kw) {
        if (c == '"') out.push_back('"');  // " → ""
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}
}

Result<std::vector<Card>> CardDao::search(const std::string& keyword,
                                         int64_t topic_id, int limit) {
    std::vector<Card> result;
    std::string sql = std::string("SELECT ") + kSelectCols
        + " FROM card WHERE id IN ("
        + "  SELECT rowid FROM card_fts WHERE card_fts MATCH ?"
        + (topic_id > 0 ? "  ) AND topic_id = ?" : "  )")
        + " ORDER BY id LIMIT ?";

    auto stmt = db_.prepare(sql);
    if (!stmt) return make_err<std::vector<Card>>(E::kDbPrepareFailed, "prepare failed");

    // 把用户输入按 FTS5 phrase 包起来，避免 OR / AND 等关键字被当查询语法
    std::string safe_kw = fts5_quote_phrase(keyword);

    int idx = 1;
    stmt.bind(idx++, safe_kw);
    if (topic_id > 0) stmt.bind(idx++, topic_id);
    stmt.bind(idx, limit);

    while (stmt.step()) {
        result.push_back(parse_row(stmt));
    }
    return result;
}

Result<void> CardDao::remove_by_topic(int64_t topic_id) {
    auto stmt = db_.prepare("DELETE FROM card WHERE topic_id = ?");
    if (!stmt) return make_err(E::kDbPrepareFailed, "prepare failed");
    stmt.bind(1, topic_id);
    if (stmt.execute() < 0) return make_err(E::kDbExecuteFailed, "delete failed");
    return Result<void>::ok();
}

}  // namespace bagu::db
