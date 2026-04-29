#include "db/statement.h"

#include <spdlog/spdlog.h>

#include <stdexcept>
#include <utility>

namespace bagu::db {

Statement::Statement(sqlite3_stmt* stmt, std::string sql)
    : stmt_(stmt), sql_(std::move(sql)) {}

Statement::~Statement() {
    if (stmt_) {
        sqlite3_finalize(stmt_);
    }
}

Statement::Statement(Statement&& other) noexcept
    : stmt_(other.stmt_), sql_(std::move(other.sql_)) {
    other.stmt_ = nullptr;
}

Statement& Statement::operator=(Statement&& other) noexcept {
    if (this != &other) {
        if (stmt_) sqlite3_finalize(stmt_);
        stmt_ = other.stmt_;
        sql_ = std::move(other.sql_);
        other.stmt_ = nullptr;
    }
    return *this;
}

void Statement::check_bind_result(int rc, int index) {
    if (rc != SQLITE_OK) {
        spdlog::error("bind failed at index {} for SQL: {} (rc={})",
            index, sql_, rc);
    }
}

void Statement::bind(int index, std::nullptr_t) {
    check_bind_result(sqlite3_bind_null(stmt_, index), index);
}

void Statement::bind(int index, int value) {
    check_bind_result(sqlite3_bind_int(stmt_, index, value), index);
}

void Statement::bind(int index, int64_t value) {
    check_bind_result(sqlite3_bind_int64(stmt_, index, value), index);
}

void Statement::bind(int index, double value) {
    check_bind_result(sqlite3_bind_double(stmt_, index, value), index);
}

void Statement::bind(int index, std::string_view value) {
    check_bind_result(
        sqlite3_bind_text(stmt_, index, value.data(),
            static_cast<int>(value.size()), SQLITE_TRANSIENT),
        index);
}

void Statement::bind(int index, const char* value) {
    if (value == nullptr) {
        bind(index, nullptr);
    } else {
        bind(index, std::string_view(value));
    }
}

void Statement::bind_blob(int index, const void* data, int size) {
    check_bind_result(
        sqlite3_bind_blob(stmt_, index, data, size, SQLITE_TRANSIENT),
        index);
}

void Statement::reset() {
    sqlite3_reset(stmt_);
}

void Statement::clear_bindings() {
    sqlite3_reset(stmt_);
    sqlite3_clear_bindings(stmt_);
}

bool Statement::step() {
    int rc = sqlite3_step(stmt_);
    if (rc == SQLITE_ROW) return true;
    if (rc == SQLITE_DONE) return false;

    spdlog::error("step failed for SQL: {} ({})", sql_, last_error());
    return false;
}

int Statement::execute() {
    int rc = sqlite3_step(stmt_);
    if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
        spdlog::error("execute failed for SQL: {} ({})", sql_, last_error());
        return -1;
    }
    sqlite3* db = sqlite3_db_handle(stmt_);
    int changes = sqlite3_changes(db);
    sqlite3_reset(stmt_);
    return changes;
}

bool Statement::is_null(int col) const {
    return sqlite3_column_type(stmt_, col) == SQLITE_NULL;
}

int Statement::column_int(int col) const {
    return sqlite3_column_int(stmt_, col);
}

int64_t Statement::column_int64(int col) const {
    return sqlite3_column_int64(stmt_, col);
}

double Statement::column_double(int col) const {
    return sqlite3_column_double(stmt_, col);
}

std::string Statement::column_text(int col) const {
    if (is_null(col)) return {};
    const unsigned char* text = sqlite3_column_text(stmt_, col);
    int size = sqlite3_column_bytes(stmt_, col);
    if (!text) return {};
    return std::string(reinterpret_cast<const char*>(text), size);
}

std::optional<std::string> Statement::column_text_opt(int col) const {
    if (is_null(col)) return std::nullopt;
    return column_text(col);
}

int Statement::column_count() const {
    return sqlite3_column_count(stmt_);
}

std::string Statement::column_name(int col) const {
    const char* name = sqlite3_column_name(stmt_, col);
    return name ? std::string(name) : std::string();
}

std::string Statement::last_error() const {
    sqlite3* db = sqlite3_db_handle(stmt_);
    return db ? sqlite3_errmsg(db) : "(no db)";
}

}  // namespace bagu::db
