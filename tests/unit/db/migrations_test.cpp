#include "db/migrations.h"

#include <gtest/gtest.h>

#include "db/database.h"

namespace bagu::db {

TEST(Migrations, RegisterAndApply_AllTablesCreated) {
    auto r = Database::open(":memory:");
    ASSERT_TRUE(r.is_ok()) << r.error().message;
    auto& db = r.value();

    register_all_migrations(db);
    auto m = db.migrate();
    ASSERT_TRUE(m.is_ok()) << m.error().message << " | " << m.error().detail;

    EXPECT_EQ(db.schema_version(), 2);

    // 验证关键表存在
    for (const auto& table : {"topic", "chapter", "card", "review",
                              "review_history", "card_fts", "schema_version"}) {
        auto stmt = db.prepare(
            "SELECT name FROM sqlite_master WHERE type IN ('table','view') AND name = ?");
        ASSERT_TRUE(stmt);
        stmt.bind(1, table);
        EXPECT_TRUE(stmt.step()) << "table missing: " << table;
    }
}

TEST(Migrations, FtsTriggers_AutoSyncCardChanges) {
    auto r = Database::open(":memory:");
    ASSERT_TRUE(r.is_ok());
    auto& db = r.value();
    register_all_migrations(db);
    ASSERT_TRUE(db.migrate().is_ok());

    // 准备 topic 让 FK 通过
    db.execute(
        "INSERT INTO topic (name, title, file_path, file_hash, imported_at, updated_at) "
        "VALUES ('t1', 'T1', '/p', 'h', 0, 0)");

    db.execute(
        "INSERT INTO card (topic_id, question, answer, created_at, updated_at) "
        "VALUES (1, 'MVCC 原理', '隐藏字段+undo+ReadView', 0, 0)");

    auto stmt = db.prepare(
        "SELECT COUNT(*) FROM card_fts WHERE card_fts MATCH 'MVCC'");
    ASSERT_TRUE(stmt);
    ASSERT_TRUE(stmt.step());
    EXPECT_EQ(stmt.column_int(0), 1);
}

}  // namespace bagu::db
