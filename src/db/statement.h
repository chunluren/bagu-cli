#pragma once

#include <sqlite3.h>

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace bagu::db {

/// SQLite prepared statement 的 RAII 封装
///
/// @code
/// auto stmt = db.prepare("SELECT name FROM topic WHERE id = ?");
/// stmt.bind(1, 42);
/// while (stmt.step()) {
///     std::string name = stmt.column_text(0);
/// }
/// @endcode
class Statement {
public:
    Statement() = default;
    Statement(sqlite3_stmt* stmt, std::string sql);
    ~Statement();

    Statement(const Statement&) = delete;
    Statement& operator=(const Statement&) = delete;

    Statement(Statement&& other) noexcept;
    Statement& operator=(Statement&& other) noexcept;

    /// 是否持有有效的 prepared statement
    bool valid() const noexcept { return stmt_ != nullptr; }
    explicit operator bool() const noexcept { return valid(); }

    sqlite3_stmt* handle() const noexcept { return stmt_; }
    const std::string& sql() const noexcept { return sql_; }

    // ===== 参数绑定（1-based 索引）=====
    void bind(int index, std::nullptr_t);
    void bind(int index, int value);
    void bind(int index, int64_t value);
    void bind(int index, double value);
    void bind(int index, std::string_view value);
    void bind(int index, const char* value);
    void bind_blob(int index, const void* data, int size);

    /// 重置执行状态（保留绑定参数）
    void reset();

    /// 重置 + 清除绑定
    void clear_bindings();

    /// 执行下一步
    /// @return true: 有行可读（SQLITE_ROW）
    ///         false: 执行完成（SQLITE_DONE）
    /// @throws 上抛错误（用 last_error()）
    bool step();

    /// 执行不返回结果集的语句（INSERT/UPDATE/DELETE/DDL）
    /// 等价于 step() 然后丢弃结果。
    /// @return 受影响的行数
    int execute();

    // ===== 结果列读取（0-based 索引）=====
    bool is_null(int col) const;
    int column_int(int col) const;
    int64_t column_int64(int col) const;
    double column_double(int col) const;
    std::string column_text(int col) const;
    std::optional<std::string> column_text_opt(int col) const;

    /// 列数（执行后才有意义）
    int column_count() const;
    std::string column_name(int col) const;

    /// 最后一次错误信息（用于错误传播）
    std::string last_error() const;

private:
    sqlite3_stmt* stmt_ = nullptr;
    std::string sql_;  // 用于错误信息

    void check_bind_result(int rc, int index);
};

}  // namespace bagu::db
