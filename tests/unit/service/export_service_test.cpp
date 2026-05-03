#include "service/export_service.h"

#include <gtest/gtest.h>

#include <sstream>

#include "db/database.h"
#include "db/migrations.h"

namespace bagu::service {

class ExportServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto r = db::Database::open(":memory:");
        ASSERT_TRUE(r.is_ok());
        db_ = std::make_unique<db::Database>(std::move(r.value()));
        db::register_all_migrations(*db_);
        ASSERT_TRUE(db_->migrate().is_ok());

        (void)db_->execute(
            "INSERT INTO topic (name, title, file_path, file_hash, "
            "imported_at, updated_at) VALUES "
            "('mysql', 'MySQL', '/p1', 'h1', 0, 0),"
            "('redis', 'Redis', '/p2', 'h2', 0, 0)");

        // mysql: 2 张 qa 卡 + 1 张 section 卡
        (void)db_->execute(
            "INSERT INTO card (topic_id, question, answer, card_type, "
            " tags, created_at, updated_at) VALUES "
            "(1, 'MVCC?', '隐藏字段+undo+ReadView', 'qa', 'tx,index', 0, 0),"
            "(1, '索引下推?', 'ICP\t在 server\n层过滤', 'qa', '', 0, 0),"  // 含 tab + LF
            "(1, '第一章', '', 'section', '', 0, 0)");

        // redis: 1 张 qa
        (void)db_->execute(
            "INSERT INTO card (topic_id, question, answer, card_type, "
            " tags, created_at, updated_at) VALUES "
            "(2, '过期策略?', '惰性+定期', 'qa', '', 0, 0)");
    }

    std::unique_ptr<db::Database> db_;
};

TEST_F(ExportServiceTest, ExportAll_OnlyQaByDefault) {
    std::ostringstream out;
    AnkiExportOptions opts;  // topic 空 = 全部
    auto r = ExportService(*db_).export_anki(opts, out);

    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().total_cards, 4);
    EXPECT_EQ(r.value().written, 3);     // 跳过 section
    EXPECT_EQ(r.value().skipped, 1);

    auto s = out.str();
    EXPECT_NE(s.find("#separator:tab"), std::string::npos);
    EXPECT_NE(s.find("MVCC?"), std::string::npos);
    EXPECT_NE(s.find("过期策略?"), std::string::npos);
    // 章节卡不应出现
    EXPECT_EQ(s.find("第一章"), std::string::npos);
}

TEST_F(ExportServiceTest, ExportAll_IncludeSection) {
    std::ostringstream out;
    AnkiExportOptions opts;
    opts.include_section_cards = true;
    auto r = ExportService(*db_).export_anki(opts, out);

    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().written, 4);
    EXPECT_NE(out.str().find("第一章"), std::string::npos);
}

TEST_F(ExportServiceTest, ExportSpecificTopic) {
    std::ostringstream out;
    AnkiExportOptions opts;
    opts.topic = "redis";
    auto r = ExportService(*db_).export_anki(opts, out);

    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().written, 1);
    EXPECT_NE(out.str().find("过期策略?"), std::string::npos);
    EXPECT_EQ(out.str().find("MVCC?"), std::string::npos);
}

TEST_F(ExportServiceTest, ExportTopicNotFound) {
    std::ostringstream out;
    AnkiExportOptions opts;
    opts.topic = "doesnotexist";
    auto r = ExportService(*db_).export_anki(opts, out);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(E::kTopicNotFound));
}

TEST_F(ExportServiceTest, EscapesTabAndNewlineInAnswer) {
    std::ostringstream out;
    AnkiExportOptions opts;
    opts.topic = "mysql";
    (void)ExportService(*db_).export_anki(opts, out);

    auto s = out.str();
    // 原 answer "ICP\t在 server\n层过滤" 应被转义
    // \t → 4 空格
    EXPECT_NE(s.find("ICP    在 server"), std::string::npos);
    // \n → <br>
    EXPECT_NE(s.find("server<br>层过滤"), std::string::npos);
}

TEST_F(ExportServiceTest, TagsContainBaguAndTopicName) {
    std::ostringstream out;
    AnkiExportOptions opts;
    opts.topic = "mysql";
    (void)ExportService(*db_).export_anki(opts, out);

    auto s = out.str();
    // 每行末尾 tag 列至少含 "bagu mysql"
    EXPECT_NE(s.find("bagu mysql"), std::string::npos);
    // 原 tags "tx,index" → 逗号转空格
    EXPECT_NE(s.find("tx index"), std::string::npos);
}

}  // namespace bagu::service
