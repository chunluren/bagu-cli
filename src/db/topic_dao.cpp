#include "db/topic_dao.h"

namespace bagu::db {

namespace {

Topic parse_row(Statement& stmt) {
    Topic t;
    t.id          = stmt.column_int64(0);
    t.name        = stmt.column_text(1);
    t.title       = stmt.column_text(2);
    t.file_path   = stmt.column_text(3);
    t.file_hash   = stmt.column_text(4);
    t.imported_at = stmt.column_int64(5);
    t.updated_at  = stmt.column_int64(6);
    return t;
}

constexpr const char* kSelectCols =
    "id, name, title, file_path, file_hash, imported_at, updated_at";

}  // namespace

Result<int64_t> TopicDao::insert(const Topic& t) {
    auto stmt = db_.prepare(
        "INSERT INTO topic (name, title, file_path, file_hash, imported_at, updated_at) "
        "VALUES (?, ?, ?, ?, ?, ?)");
    if (!stmt) return make_err<int64_t>(E::kDbPrepareFailed, "prepare insert topic failed");

    stmt.bind(1, t.name);
    stmt.bind(2, t.title);
    stmt.bind(3, t.file_path);
    stmt.bind(4, t.file_hash);
    stmt.bind(5, t.imported_at);
    stmt.bind(6, t.updated_at);

    if (stmt.execute() < 0) {
        return make_err<int64_t>(E::kDbExecuteFailed, "insert topic failed");
    }
    return db_.last_insert_rowid();
}

Result<void> TopicDao::update(const Topic& t) {
    auto stmt = db_.prepare(
        "UPDATE topic SET name=?, title=?, file_path=?, file_hash=?, "
        "imported_at=?, updated_at=? WHERE id=?");
    if (!stmt) return make_err(E::kDbPrepareFailed, "prepare update topic failed");

    stmt.bind(1, t.name);
    stmt.bind(2, t.title);
    stmt.bind(3, t.file_path);
    stmt.bind(4, t.file_hash);
    stmt.bind(5, t.imported_at);
    stmt.bind(6, t.updated_at);
    stmt.bind(7, t.id);

    if (stmt.execute() < 0) {
        return make_err(E::kDbExecuteFailed, "update topic failed");
    }
    return Result<void>::ok();
}

Result<std::optional<Topic>> TopicDao::find_by_name(const std::string& name) {
    auto stmt = db_.prepare(std::string("SELECT ") + kSelectCols
        + " FROM topic WHERE name = ?");
    if (!stmt) return make_err<std::optional<Topic>>(E::kDbPrepareFailed, "prepare failed");

    stmt.bind(1, name);
    if (!stmt.step()) return std::optional<Topic>(std::nullopt);
    return std::optional<Topic>(parse_row(stmt));
}

Result<std::optional<Topic>> TopicDao::find_by_id(int64_t id) {
    auto stmt = db_.prepare(std::string("SELECT ") + kSelectCols
        + " FROM topic WHERE id = ?");
    if (!stmt) return make_err<std::optional<Topic>>(E::kDbPrepareFailed, "prepare failed");

    stmt.bind(1, id);
    if (!stmt.step()) return std::optional<Topic>(std::nullopt);
    return std::optional<Topic>(parse_row(stmt));
}

Result<std::vector<Topic>> TopicDao::find_all() {
    std::vector<Topic> result;
    auto stmt = db_.prepare(std::string("SELECT ") + kSelectCols
        + " FROM topic ORDER BY name");
    if (!stmt) return make_err<std::vector<Topic>>(E::kDbPrepareFailed, "prepare failed");

    while (stmt.step()) {
        result.push_back(parse_row(stmt));
    }
    return result;
}

Result<void> TopicDao::remove(int64_t id) {
    auto stmt = db_.prepare("DELETE FROM topic WHERE id = ?");
    if (!stmt) return make_err(E::kDbPrepareFailed, "prepare failed");
    stmt.bind(1, id);
    if (stmt.execute() < 0) return make_err(E::kDbExecuteFailed, "delete failed");
    return Result<void>::ok();
}

}  // namespace bagu::db
