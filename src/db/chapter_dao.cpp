#include "db/chapter_dao.h"

namespace bagu::db {

Result<int64_t> ChapterDao::insert(const Chapter& c) {
    auto stmt = db_.prepare(
        "INSERT INTO chapter (topic_id, name, chapter_no, parent_id) "
        "VALUES (?, ?, ?, ?)");
    if (!stmt) return make_err<int64_t>(E::kDbPrepareFailed, "prepare failed");

    stmt.bind(1, c.topic_id);
    stmt.bind(2, c.name);
    stmt.bind(3, c.chapter_no);
    if (c.parent_id.has_value()) {
        stmt.bind(4, *c.parent_id);
    } else {
        stmt.bind(4, nullptr);
    }

    if (stmt.execute() < 0) {
        return make_err<int64_t>(E::kDbExecuteFailed, "insert chapter failed");
    }
    return db_.last_insert_rowid();
}

Result<std::vector<Chapter>> ChapterDao::find_by_topic(int64_t topic_id) {
    std::vector<Chapter> result;
    auto stmt = db_.prepare(
        "SELECT id, topic_id, name, chapter_no, parent_id "
        "FROM chapter WHERE topic_id = ? "
        "ORDER BY chapter_no, id");
    if (!stmt) return make_err<std::vector<Chapter>>(E::kDbPrepareFailed, "prepare failed");

    stmt.bind(1, topic_id);
    while (stmt.step()) {
        Chapter c;
        c.id         = stmt.column_int64(0);
        c.topic_id   = stmt.column_int64(1);
        c.name       = stmt.column_text(2);
        c.chapter_no = stmt.column_int(3);
        if (!stmt.is_null(4)) {
            c.parent_id = stmt.column_int64(4);
        }
        result.push_back(std::move(c));
    }
    return result;
}

Result<int64_t> ChapterDao::count_by_topic(int64_t topic_id) {
    auto stmt = db_.prepare("SELECT COUNT(*) FROM chapter WHERE topic_id = ?");
    if (!stmt) return make_err<int64_t>(E::kDbPrepareFailed, "prepare failed");
    stmt.bind(1, topic_id);
    if (!stmt.step()) return int64_t{0};
    return stmt.column_int64(0);
}

}  // namespace bagu::db
