#include "db/card_dao.h"

#include <gtest/gtest.h>

#include "db/database.h"
#include "db/migrations.h"
#include "db/topic_dao.h"

namespace bagu::db {

class CardDaoTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto r = Database::open(":memory:");
        ASSERT_TRUE(r.is_ok());
        db_ = std::make_unique<Database>(std::move(r.value()));
        register_all_migrations(*db_);
        ASSERT_TRUE(db_->migrate().is_ok());
        cards_ = std::make_unique<CardDao>(*db_);

        // 准备一个 topic 用于 FK
        TopicDao tdao(*db_);
        Topic t;
        t.name = "mysql"; t.title = "MySQL"; t.file_path = "/p"; t.file_hash = "h";
        t.imported_at = 0; t.updated_at = 0;
        topic_id_ = tdao.insert(t).value();
    }

    Card make_card(const std::string& q, const std::string& a) {
        Card c;
        c.topic_id = topic_id_;
        c.question = q;
        c.answer = a;
        c.created_at = 0;
        c.updated_at = 0;
        return c;
    }

    std::unique_ptr<Database> db_;
    std::unique_ptr<CardDao> cards_;
    int64_t topic_id_ = 0;
};

TEST_F(CardDaoTest, InsertAndFind_RoundTrip) {
    auto id_r = cards_->insert(make_card("MVCC?", "多版本并发控制"));
    ASSERT_TRUE(id_r.is_ok());

    auto r = cards_->find_by_id(id_r.value());
    ASSERT_TRUE(r.is_ok());
    ASSERT_TRUE(r.value().has_value());
    EXPECT_EQ(r.value()->question, "MVCC?");
    EXPECT_EQ(r.value()->answer, "多版本并发控制");
    EXPECT_EQ(r.value()->topic_id, topic_id_);
}

TEST_F(CardDaoTest, FindByTopic_ReturnsAll) {
    cards_->insert(make_card("Q1", "A1"));
    cards_->insert(make_card("Q2", "A2"));
    cards_->insert(make_card("Q3", "A3"));

    auto r = cards_->find_by_topic(topic_id_);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().size(), 3u);
}

TEST_F(CardDaoTest, CountByTopic_Returns) {
    EXPECT_EQ(cards_->count_by_topic(topic_id_).value(), 0);
    cards_->insert(make_card("Q1", "A1"));
    cards_->insert(make_card("Q2", "A2"));
    EXPECT_EQ(cards_->count_by_topic(topic_id_).value(), 2);
}

TEST_F(CardDaoTest, Search_ReturnsMatchingCards) {
    cards_->insert(make_card("MVCC 实现?", "隐藏字段"));
    cards_->insert(make_card("索引?", "B+ 树"));
    cards_->insert(make_card("MVCC 何时用?", "并发场景"));

    auto r = cards_->search("MVCC", 0, 20);
    ASSERT_TRUE(r.is_ok()) << r.error().message;
    EXPECT_EQ(r.value().size(), 2u);
}

TEST_F(CardDaoTest, Search_RestrictedToTopic) {
    // 添加另一个 topic 的卡片
    TopicDao tdao(*db_);
    Topic t2;
    t2.name = "other"; t2.title = "Other"; t2.file_path = "/p2"; t2.file_hash = "h2";
    t2.imported_at = 0; t2.updated_at = 0;
    int64_t other_id = tdao.insert(t2).value();

    cards_->insert(make_card("MVCC q1", "a1"));

    Card c2 = make_card("MVCC q2", "a2");
    c2.topic_id = other_id;
    cards_->insert(c2);

    auto r = cards_->search("MVCC", topic_id_, 20);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().size(), 1u);
    EXPECT_EQ(r.value()[0].topic_id, topic_id_);
}

TEST_F(CardDaoTest, RemoveByTopic_DeletesAll) {
    cards_->insert(make_card("a", "b"));
    cards_->insert(make_card("c", "d"));

    ASSERT_TRUE(cards_->remove_by_topic(topic_id_).is_ok());
    EXPECT_EQ(cards_->count_by_topic(topic_id_).value(), 0);
}

TEST_F(CardDaoTest, Search_KeywordWithFts5Operators_DoesNotErrorOut) {
    // FTS5 关键字（OR / AND / NEAR / *）必须被当字面量处理
    cards_->insert(make_card("foo OR bar", "baz"));
    cards_->insert(make_card("hello", "world"));

    // 这些 keyword 在原始 FTS5 语法中是关键字，过去会报 malformed
    for (const auto& kw : {"OR", "AND", "foo OR", "*", "-bar", "NEAR"}) {
        auto r = cards_->search(kw, 0, 10);
        ASSERT_TRUE(r.is_ok()) << "search failed on keyword='" << kw
                               << "' err=" << (r.is_err() ? r.error().message : "");
    }
}

TEST_F(CardDaoTest, Search_KeywordWithDoubleQuote_HandledSafely) {
    cards_->insert(make_card("说 \"hello\" 即可", "answer"));
    auto r = cards_->search("\"hello\"", 0, 10);
    ASSERT_TRUE(r.is_ok());
    // 至少不崩溃；命中与否取决于分词，关键是不抛错误
}

}  // namespace bagu::db
