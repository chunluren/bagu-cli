#include "db/database.h"

#include <gtest/gtest.h>

#include <filesystem>

namespace bagu::db {

namespace fs = std::filesystem;

class DatabaseTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto r = Database::open(":memory:");
        ASSERT_TRUE(r.is_ok()) << r.error().message << " | detail: " << r.error().detail;
        db_ = std::make_unique<Database>(std::move(r.value()));
    }

    std::unique_ptr<Database> db_;
};

TEST_F(DatabaseTest, Open_InMemory_Succeeds) {
    ASSERT_NE(db_, nullptr);
    EXPECT_TRUE(db_->is_open());
}

TEST_F(DatabaseTest, Execute_CreateTable_Succeeds) {
    auto r = db_->execute("CREATE TABLE t (id INTEGER PRIMARY KEY, v TEXT)");
    EXPECT_TRUE(r.is_ok());
}

TEST_F(DatabaseTest, Execute_InvalidSql_ReturnsError) {
    auto r = db_->execute("CREATE NOT_A_KEYWORD foo");
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(E::kDbExecuteFailed));
}

TEST_F(DatabaseTest, InsertAndQuery_RoundTrip) {
    db_->execute("CREATE TABLE t (id INTEGER PRIMARY KEY, v TEXT)");

    auto stmt = db_->prepare("INSERT INTO t (v) VALUES (?)");
    ASSERT_TRUE(stmt);
    stmt.bind(1, "hello");
    EXPECT_EQ(stmt.execute(), 1);

    EXPECT_EQ(db_->last_insert_rowid(), 1);

    auto query = db_->prepare("SELECT id, v FROM t WHERE id = ?");
    query.bind(1, static_cast<int64_t>(1));
    ASSERT_TRUE(query.step());
    EXPECT_EQ(query.column_int(0), 1);
    EXPECT_EQ(query.column_text(1), "hello");
    EXPECT_FALSE(query.step());
}

TEST_F(DatabaseTest, Transaction_Commit_PersistsData) {
    db_->execute("CREATE TABLE t (id INTEGER PRIMARY KEY)");

    {
        auto txn = db_->begin();
        db_->execute("INSERT INTO t VALUES (1)");
        db_->execute("INSERT INTO t VALUES (2)");
        ASSERT_TRUE(txn.commit().is_ok());
    }

    auto stmt = db_->prepare("SELECT COUNT(*) FROM t");
    ASSERT_TRUE(stmt.step());
    EXPECT_EQ(stmt.column_int(0), 2);
}

TEST_F(DatabaseTest, Transaction_AutoRollback_DiscardsChanges) {
    db_->execute("CREATE TABLE t (id INTEGER PRIMARY KEY)");

    {
        auto txn = db_->begin();
        db_->execute("INSERT INTO t VALUES (1)");
        db_->execute("INSERT INTO t VALUES (2)");
        // 不 commit，析构自动 rollback
    }

    auto stmt = db_->prepare("SELECT COUNT(*) FROM t");
    ASSERT_TRUE(stmt.step());
    EXPECT_EQ(stmt.column_int(0), 0);
}

TEST_F(DatabaseTest, SchemaVersion_NoMigrations_ReturnsZero) {
    EXPECT_EQ(db_->schema_version(), 0);
}

TEST_F(DatabaseTest, Migrate_AppliesAndRecordsVersion) {
    db_->register_migration(1, "create users",
        "CREATE TABLE users (id INTEGER PRIMARY KEY, name TEXT)");
    db_->register_migration(2, "add email",
        "ALTER TABLE users ADD COLUMN email TEXT");

    auto r = db_->migrate();
    ASSERT_TRUE(r.is_ok()) << r.error().message;
    EXPECT_EQ(db_->schema_version(), 2);

    // 验证表存在且有 email 列
    auto stmt = db_->prepare("INSERT INTO users (name, email) VALUES (?, ?)");
    ASSERT_TRUE(stmt);
    stmt.bind(1, "alice");
    stmt.bind(2, "alice@example.com");
    EXPECT_EQ(stmt.execute(), 1);
}

TEST_F(DatabaseTest, Migrate_TwiceIsIdempotent) {
    db_->register_migration(1, "init",
        "CREATE TABLE x (id INTEGER)");
    ASSERT_TRUE(db_->migrate().is_ok());
    EXPECT_EQ(db_->schema_version(), 1);

    // 再 migrate，schema_version 不变
    ASSERT_TRUE(db_->migrate().is_ok());
    EXPECT_EQ(db_->schema_version(), 1);
}

TEST_F(DatabaseTest, FileBased_PersistsAcrossOpens) {
    auto tmp = fs::temp_directory_path() / ("bagu-test-" + std::to_string(::getpid()) + ".db");
    fs::remove(tmp);

    {
        auto r = Database::open(tmp);
        ASSERT_TRUE(r.is_ok());
        auto& db = r.value();
        db.execute("CREATE TABLE t (v INTEGER)");
        db.execute("INSERT INTO t VALUES (123)");
    }

    {
        auto r = Database::open(tmp);
        ASSERT_TRUE(r.is_ok());
        auto& db = r.value();
        auto stmt = db.prepare("SELECT v FROM t");
        ASSERT_TRUE(stmt.step());
        EXPECT_EQ(stmt.column_int(0), 123);
    }

    fs::remove(tmp);
    fs::remove(tmp.string() + "-wal");
    fs::remove(tmp.string() + "-shm");
}

}  // namespace bagu::db
