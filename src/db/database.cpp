#include "db/database.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <utility>

namespace bagu::db {

// ============================================================================
// Transaction
// ============================================================================

Transaction::Transaction(Database& db) : db_(db) {
    auto r = db_.execute("BEGIN");
    if (r.is_err()) {
        spdlog::error("Failed to BEGIN transaction: {}", r.error().message);
        finished_ = true;
    }
}

Transaction::~Transaction() {
    if (!finished_) {
        rollback();
    }
}

Result<void> Transaction::commit() {
    if (finished_) return make_err(E::kInternal, "transaction already finished");
    auto r = db_.execute("COMMIT");
    finished_ = true;
    return r;
}

void Transaction::rollback() {
    if (finished_) return;
    auto r = db_.execute("ROLLBACK");
    if (r.is_err()) {
        spdlog::warn("ROLLBACK failed: {}", r.error().message);
    }
    finished_ = true;
}

// ============================================================================
// Database
// ============================================================================

Database::~Database() {
    close();
}

Database::Database(Database&& other) noexcept
    : db_(other.db_), migrations_(std::move(other.migrations_)) {
    other.db_ = nullptr;
}

Database& Database::operator=(Database&& other) noexcept {
    if (this != &other) {
        close();
        db_ = other.db_;
        migrations_ = std::move(other.migrations_);
        other.db_ = nullptr;
    }
    return *this;
}

void Database::close() {
    if (db_) {
        sqlite3_close_v2(db_);
        db_ = nullptr;
    }
}

Result<Database> Database::open(const std::filesystem::path& path) {
    sqlite3* raw = nullptr;
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX;
    int rc = sqlite3_open_v2(path.string().c_str(), &raw, flags, nullptr);
    if (rc != SQLITE_OK) {
        std::string msg = raw ? sqlite3_errmsg(raw) : "unknown";
        if (raw) sqlite3_close_v2(raw);
        return make_err<Database>(E::kDbOpenFailed,
            "无法打开数据库",
            "path=" + path.string() + " err=" + msg);
    }

    Database db(raw);
    auto pragma_r = db.apply_pragmas();
    if (pragma_r.is_err()) {
        return make_err<Database>(E::kDbOpenFailed,
            std::string("应用 PRAGMA 失败: ") + pragma_r.error().message,
            pragma_r.error().detail);
    }
    return db;
}

Result<void> Database::apply_pragmas() {
    // WAL 模式仅文件库支持（:memory: 不支持，会无声忽略，所以直接执行）
    // PRAGMA journal_mode = WAL 即使无效也返回当前模式而非错误
    auto r = execute("PRAGMA journal_mode = WAL");
    if (r.is_err()) return r;
    r = execute("PRAGMA synchronous = NORMAL");
    if (r.is_err()) return r;
    r = execute("PRAGMA foreign_keys = ON");
    if (r.is_err()) return r;
    r = execute("PRAGMA temp_store = MEMORY");
    if (r.is_err()) return r;
    r = execute("PRAGMA cache_size = -10000");  // 约 10MB
    if (r.is_err()) return r;
    return Result<void>::ok();
}

Result<void> Database::execute(std::string_view sql) {
    if (!db_) return make_err(E::kDbExecuteFailed, "database not open");

    char* errmsg = nullptr;
    int rc = sqlite3_exec(db_, std::string(sql).c_str(), nullptr, nullptr, &errmsg);
    if (rc != SQLITE_OK) {
        std::string msg = errmsg ? errmsg : "unknown";
        sqlite3_free(errmsg);
        return make_err(E::kDbExecuteFailed, "SQL 执行失败",
            "sql=" + std::string(sql) + " err=" + msg);
    }
    return Result<void>::ok();
}

Statement Database::prepare(std::string_view sql) {
    if (!db_) {
        spdlog::error("prepare on closed database");
        return Statement();
    }

    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(db_, sql.data(),
        static_cast<int>(sql.size()), &stmt, nullptr);
    if (rc != SQLITE_OK) {
        spdlog::error("prepare failed: {} ({})", last_error(), sql);
        return Statement();
    }
    return Statement(stmt, std::string(sql));
}

Transaction Database::begin() {
    return Transaction(*this);
}

int64_t Database::last_insert_rowid() const {
    return db_ ? sqlite3_last_insert_rowid(db_) : 0;
}

int Database::changes() const {
    return db_ ? sqlite3_changes(db_) : 0;
}

std::string Database::last_error() const {
    return db_ ? sqlite3_errmsg(db_) : "(no db)";
}

void Database::register_migration(int version, std::string description, std::string sql) {
    migrations_.push_back({version, std::move(description), std::move(sql)});
    std::sort(migrations_.begin(), migrations_.end(),
        [](const Migration& a, const Migration& b) { return a.version < b.version; });
}

Result<void> Database::ensure_schema_version_table() {
    return execute(
        "CREATE TABLE IF NOT EXISTS schema_version ("
        "  version INTEGER PRIMARY KEY,"
        "  applied_at INTEGER NOT NULL,"
        "  description TEXT"
        ")");
}

int Database::schema_version() const {
    if (!db_) return 0;

    sqlite3_stmt* stmt = nullptr;
    const char* sql = "SELECT MAX(version) FROM schema_version";
    int rc = sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr);
    if (rc != SQLITE_OK) return 0;  // 表不存在视为版本 0

    int version = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW &&
        sqlite3_column_type(stmt, 0) != SQLITE_NULL) {
        version = sqlite3_column_int(stmt, 0);
    }
    sqlite3_finalize(stmt);
    return version;
}

Result<void> Database::migrate() {
    if (!db_) return make_err(E::kDbMigrationFailed, "database not open");

    auto r = ensure_schema_version_table();
    if (r.is_err()) return r;

    int current = schema_version();
    spdlog::debug("current schema version: {}", current);

    Transaction txn(*this);
    for (const auto& m : migrations_) {
        if (m.version <= current) continue;

        spdlog::info("applying migration {} ({})", m.version, m.description);
        auto exec_r = execute(m.sql);
        if (exec_r.is_err()) {
            return make_err(E::kDbMigrationFailed,
                "migration 失败 v" + std::to_string(m.version),
                exec_r.error().message);
        }

        auto stmt = prepare(
            "INSERT INTO schema_version (version, applied_at, description) "
            "VALUES (?, strftime('%s','now'), ?)");
        if (!stmt) {
            return make_err(E::kDbMigrationFailed, "prepare schema_version insert failed");
        }
        stmt.bind(1, m.version);
        stmt.bind(2, m.description);
        if (stmt.execute() < 0) {
            return make_err(E::kDbMigrationFailed, "insert schema_version failed");
        }
    }
    return txn.commit();
}

}  // namespace bagu::db
