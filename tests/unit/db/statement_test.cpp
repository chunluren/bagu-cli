#include "db/statement.h"

#include <gtest/gtest.h>

#include "db/database.h"

namespace bagu::db {

class StatementTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto r = Database::open(":memory:");
        ASSERT_TRUE(r.is_ok());
        db_ = std::make_unique<Database>(std::move(r.value()));
        db_->execute(
            "CREATE TABLE t ("
            "  id INTEGER PRIMARY KEY,"
            "  name TEXT,"
            "  age INTEGER,"
            "  height REAL"
            ")");
    }

    std::unique_ptr<Database> db_;
};

TEST_F(StatementTest, BindAllTypes_RoundTrip) {
    auto ins = db_->prepare("INSERT INTO t (name, age, height) VALUES (?, ?, ?)");
    ASSERT_TRUE(ins);
    ins.bind(1, "alice");
    ins.bind(2, 30);
    ins.bind(3, 1.65);
    EXPECT_EQ(ins.execute(), 1);

    auto sel = db_->prepare("SELECT name, age, height FROM t");
    ASSERT_TRUE(sel.step());
    EXPECT_EQ(sel.column_text(0), "alice");
    EXPECT_EQ(sel.column_int(1), 30);
    EXPECT_DOUBLE_EQ(sel.column_double(2), 1.65);
}

TEST_F(StatementTest, BindNull_StoresNull) {
    auto ins = db_->prepare("INSERT INTO t (name) VALUES (?)");
    ins.bind(1, nullptr);
    EXPECT_EQ(ins.execute(), 1);

    auto sel = db_->prepare("SELECT name FROM t");
    ASSERT_TRUE(sel.step());
    EXPECT_TRUE(sel.is_null(0));
}

TEST_F(StatementTest, ColumnTextOpt_NullColumn_ReturnsNullopt) {
    db_->execute("INSERT INTO t (name) VALUES (NULL)");
    auto sel = db_->prepare("SELECT name FROM t");
    ASSERT_TRUE(sel.step());
    EXPECT_FALSE(sel.column_text_opt(0).has_value());
}

TEST_F(StatementTest, ColumnTextOpt_NonNull_HasValue) {
    db_->execute("INSERT INTO t (name) VALUES ('bob')");
    auto sel = db_->prepare("SELECT name FROM t");
    ASSERT_TRUE(sel.step());
    auto v = sel.column_text_opt(0);
    ASSERT_TRUE(v.has_value());
    EXPECT_EQ(*v, "bob");
}

TEST_F(StatementTest, Reset_AllowsReExecute) {
    auto ins = db_->prepare("INSERT INTO t (name) VALUES (?)");
    for (const auto& name : {"a", "b", "c"}) {
        ins.bind(1, name);
        EXPECT_EQ(ins.execute(), 1);
    }

    auto sel = db_->prepare("SELECT COUNT(*) FROM t");
    ASSERT_TRUE(sel.step());
    EXPECT_EQ(sel.column_int(0), 3);
}

TEST_F(StatementTest, MoveConstructor_TransfersOwnership) {
    auto stmt1 = db_->prepare("SELECT 1");
    ASSERT_TRUE(stmt1);
    auto stmt2 = std::move(stmt1);
    EXPECT_FALSE(stmt1.valid());
    EXPECT_TRUE(stmt2.valid());
    EXPECT_TRUE(stmt2.step());
    EXPECT_EQ(stmt2.column_int(0), 1);
}

TEST_F(StatementTest, InvalidSql_ReturnsInvalidStatement) {
    auto stmt = db_->prepare("SELECT FROM WHERE");
    EXPECT_FALSE(stmt.valid());
    EXPECT_FALSE(static_cast<bool>(stmt));
}

TEST_F(StatementTest, ColumnCount_AfterStep_ReturnsCorrect) {
    db_->execute("INSERT INTO t (name, age) VALUES ('x', 1)");
    auto sel = db_->prepare("SELECT name, age FROM t");
    ASSERT_TRUE(sel.step());
    EXPECT_EQ(sel.column_count(), 2);
}

}  // namespace bagu::db
