#include "db/card_dao.h"

#include <spdlog/spdlog.h>

namespace bagu::db {

namespace {

constexpr const char* kSelectCols =
    "id, topic_id, chapter_id, question, answer, "
    "difficulty, tags, source_line, card_type, created_at, updated_at, "
    "IFNULL(stable_key, '')";

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
    c.stable_key = stmt.column_text(11);
    return c;
}

Result<int64_t> CardDao::insert(const Card& c) {
    auto stmt = db_.prepare(
        "INSERT INTO card (topic_id, chapter_id, question, answer, "
        "difficulty, tags, source_line, card_type, created_at, updated_at, "
        "stable_key) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
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
    if (c.stable_key.empty()) {
        stmt.bind(11, nullptr);
    } else {
        stmt.bind(11, c.stable_key);
    }

    if (stmt.execute() < 0) {
        return make_err<int64_t>(E::kDbExecuteFailed, "insert card failed");
    }
    return db_.last_insert_rowid();
}

Result<int64_t> CardDao::upsert_by_stable_key(const Card& c) {
    if (c.stable_key.empty()) {
        return make_err<int64_t>(E::kArgRequired,
            "upsert_by_stable_key 要求 stable_key 非空");
    }

    // 先查现有 id
    int64_t existing_id = 0;
    {
        auto sel = db_.prepare("SELECT id FROM card WHERE stable_key = ?");
        if (!sel) return make_err<int64_t>(E::kDbPrepareFailed, "prepare select failed");
        sel.bind(1, c.stable_key);
        if (sel.step()) existing_id = sel.column_int64(0);
    }

    if (existing_id == 0) {
        // 不存在 → INSERT
        return insert(c);
    }

    // 存在 → UPDATE（保留 id；review/review_history 通过 FK 关联，不动）
    auto upd = db_.prepare(
        "UPDATE card SET topic_id = ?, chapter_id = ?, question = ?, "
        "answer = ?, difficulty = ?, tags = ?, source_line = ?, "
        "card_type = ?, updated_at = ? WHERE id = ?");
    if (!upd) return make_err<int64_t>(E::kDbPrepareFailed, "prepare update failed");

    upd.bind(1, c.topic_id);
    if (c.chapter_id.has_value()) {
        upd.bind(2, *c.chapter_id);
    } else {
        upd.bind(2, nullptr);
    }
    upd.bind(3, c.question);
    upd.bind(4, c.answer);
    upd.bind(5, c.difficulty);
    upd.bind(6, c.tags);
    upd.bind(7, c.source_line);
    upd.bind(8, c.card_type);
    upd.bind(9, c.updated_at);
    upd.bind(10, existing_id);

    if (upd.execute() < 0) {
        return make_err<int64_t>(E::kDbExecuteFailed, "update card failed");
    }
    return existing_id;
}

Result<int64_t> CardDao::delete_topic_cards_not_in(
    int64_t topic_id, const std::vector<std::string>& keep_keys) {
    // 用临时表承载 keep_keys，避免动态拼 SQL。
    // 在事务里跑很安全；表名带 pid 避免并发冲突（FULLMUTEX 已经串行了，但保险）
    std::string tmp = "tmp_keep_keys_" + std::to_string(reinterpret_cast<uintptr_t>(&topic_id));
    auto cleanup = [&]() {
        (void)db_.execute("DROP TABLE IF EXISTS " + tmp);
    };
    cleanup();

    auto cr = db_.execute("CREATE TEMP TABLE " + tmp + " (k TEXT PRIMARY KEY)");
    if (cr.is_err()) return make_err<int64_t>(E::kDbExecuteFailed,
        "create temp table failed", cr.error().message);

    {
        auto ins = db_.prepare("INSERT OR IGNORE INTO " + tmp + " (k) VALUES (?)");
        if (!ins) {
            cleanup();
            return make_err<int64_t>(E::kDbPrepareFailed, "prepare insert temp failed");
        }
        for (const auto& k : keep_keys) {
            if (k.empty()) continue;
            ins.reset();
            ins.bind(1, k);
            if (ins.execute() < 0) {
                cleanup();
                return make_err<int64_t>(E::kDbExecuteFailed, "insert temp failed");
            }
        }
    }

    auto del = db_.prepare(
        "DELETE FROM card WHERE topic_id = ? "
        "AND (stable_key IS NULL OR stable_key NOT IN (SELECT k FROM " + tmp + "))");
    if (!del) {
        cleanup();
        return make_err<int64_t>(E::kDbPrepareFailed, "prepare delete failed");
    }
    del.bind(1, topic_id);
    int64_t affected = del.execute();
    cleanup();
    if (affected < 0) return make_err<int64_t>(E::kDbExecuteFailed, "delete failed");
    return affected;
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
