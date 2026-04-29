#include "db/topic_dao.h"

#include <gtest/gtest.h>

#include "db/database.h"
#include "db/migrations.h"

namespace bagu::db {

class TopicDaoTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto r = Database::open(":memory:");
        ASSERT_TRUE(r.is_ok());
        db_ = std::make_unique<Database>(std::move(r.value()));
        register_all_migrations(*db_);
        ASSERT_TRUE(db_->migrate().is_ok());
        dao_ = std::make_unique<TopicDao>(*db_);
    }

    Topic make_topic(const std::string& name) {
        Topic t;
        t.name = name;
        t.title = name + " title";
        t.file_path = "/path/" + name + ".md";
        t.file_hash = "deadbeef";
        t.imported_at = 1700000000;
        t.updated_at = 1700000000;
        return t;
    }

    std::unique_ptr<Database> db_;
    std::unique_ptr<TopicDao> dao_;
};

TEST_F(TopicDaoTest, Insert_ReturnsId) {
    auto r = dao_->insert(make_topic("mysql"));
    ASSERT_TRUE(r.is_ok());
    EXPECT_GT(r.value(), 0);
}

TEST_F(TopicDaoTest, FindByName_AfterInsert_Returns) {
    dao_->insert(make_topic("redis"));
    auto r = dao_->find_by_name("redis");
    ASSERT_TRUE(r.is_ok());
    ASSERT_TRUE(r.value().has_value());
    EXPECT_EQ(r.value()->name, "redis");
    EXPECT_EQ(r.value()->title, "redis title");
}

TEST_F(TopicDaoTest, FindByName_Missing_ReturnsNullopt) {
    auto r = dao_->find_by_name("notexist");
    ASSERT_TRUE(r.is_ok());
    EXPECT_FALSE(r.value().has_value());
}

TEST_F(TopicDaoTest, FindAll_ReturnsAllInsertedSorted) {
    dao_->insert(make_topic("redis"));
    dao_->insert(make_topic("mysql"));
    dao_->insert(make_topic("cpp-net"));
    auto r = dao_->find_all();
    ASSERT_TRUE(r.is_ok());
    ASSERT_EQ(r.value().size(), 3u);
    EXPECT_EQ(r.value()[0].name, "cpp-net");  // 字母序
    EXPECT_EQ(r.value()[1].name, "mysql");
    EXPECT_EQ(r.value()[2].name, "redis");
}

TEST_F(TopicDaoTest, Update_ChangesFields) {
    auto id_r = dao_->insert(make_topic("mysql"));
    ASSERT_TRUE(id_r.is_ok());

    Topic t = make_topic("mysql");
    t.id = id_r.value();
    t.title = "MySQL Updated";
    t.file_hash = "newhash";
    auto u = dao_->update(t);
    ASSERT_TRUE(u.is_ok());

    auto r = dao_->find_by_id(t.id);
    ASSERT_TRUE(r.is_ok());
    ASSERT_TRUE(r.value().has_value());
    EXPECT_EQ(r.value()->title, "MySQL Updated");
    EXPECT_EQ(r.value()->file_hash, "newhash");
}

TEST_F(TopicDaoTest, Remove_DeletesRow) {
    auto id_r = dao_->insert(make_topic("mysql"));
    ASSERT_TRUE(dao_->remove(id_r.value()).is_ok());

    auto r = dao_->find_by_name("mysql");
    EXPECT_FALSE(r.value().has_value());
}

TEST_F(TopicDaoTest, Insert_DuplicateName_Fails) {
    dao_->insert(make_topic("mysql"));
    auto r = dao_->insert(make_topic("mysql"));
    EXPECT_TRUE(r.is_err());  // UNIQUE 约束
}

}  // namespace bagu::db
