#pragma once

#include <sqlite3.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

#include "bagu/result.h"
#include "db/statement.h"

namespace bagu::db {

class Database;

/// RAII 事务对象。析构时若未 commit 则自动 rollback。
class Transaction {
public:
    explicit Transaction(Database& db);
    ~Transaction();

    Transaction(const Transaction&) = delete;
    Transaction& operator=(const Transaction&) = delete;
    Transaction(Transaction&&) = delete;

    /// 提交事务
    Result<void> commit();

    /// 显式回滚（一般用 RAII 即可）
    void rollback();

private:
    Database& db_;
    bool finished_ = false;
};

/// SQLite 数据库连接封装
///
/// @code
/// auto db_r = Database::open("/path/to/bagu.db");
/// if (db_r.is_err()) { ... }
/// auto& db = db_r.value();
///
/// db.execute("CREATE TABLE x (id INTEGER)");
///
/// {
///     auto txn = db.begin();
///     auto stmt = db.prepare("INSERT INTO x VALUES (?)");
///     stmt.bind(1, 42);
///     stmt.execute();
///     txn.commit();
/// }
/// @endcode
class Database {
public:
    Database() = default;
    ~Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    Database(Database&& other) noexcept;
    Database& operator=(Database&& other) noexcept;

    /// 打开数据库文件，自动启用 WAL + 推荐 PRAGMA
    /// @param path 文件路径，":memory:" 表示内存数据库
    static Result<Database> open(const std::filesystem::path& path);

    /// 是否已打开
    bool is_open() const noexcept { return db_ != nullptr; }
    explicit operator bool() const noexcept { return is_open(); }

    /// 关闭连接
    void close();

    /// 直接执行 SQL（无返回值的 DDL / 简单语句）
    Result<void> execute(std::string_view sql);

    /// 准备 SQL 语句
    /// 失败返回 invalid Statement（valid() == false）
    Statement prepare(std::string_view sql);

    /// 开始事务
    Transaction begin();

    /// 最后插入行的 rowid
    int64_t last_insert_rowid() const;

    /// 最后一次操作影响的行数
    int changes() const;

    /// 最后一次错误信息
    std::string last_error() const;

    /// 应用所有未执行的 schema migrations
    /// migrations 通过 register_migration 注册
    Result<void> migrate();

    /// 注册一个 migration（version 必须严格递增）
    /// 通常在程序启动时一次性注册全部
    void register_migration(int version, std::string description, std::string sql);

    /// 当前 schema 版本（无表时返回 0）
    int schema_version() const;

    /// 内部：原始句柄（DAO 层用）
    sqlite3* handle() noexcept { return db_; }

private:
    explicit Database(sqlite3* db) : db_(db) {}

    /// 打开后立即执行的初始化 PRAGMA
    Result<void> apply_pragmas();

    /// 创建 schema_version 表（首次 migrate 时调用）
    Result<void> ensure_schema_version_table();

    sqlite3* db_ = nullptr;

    struct Migration {
        int version;
        std::string description;
        std::string sql;
    };
    std::vector<Migration> migrations_;
};

}  // namespace bagu::db
