#include "service/import_service.h"

#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>

#include "db/card_dao.h"
#include "db/database.h"
#include "db/migrations.h"
#include "db/topic_dao.h"

namespace bagu::service {

namespace fs = std::filesystem;

class ImportServiceTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto r = db::Database::open(":memory:");
        ASSERT_TRUE(r.is_ok());
        db_ = std::make_unique<db::Database>(std::move(r.value()));
        db::register_all_migrations(*db_);
        ASSERT_TRUE(db_->migrate().is_ok());
        svc_ = std::make_unique<ImportService>(*db_);

        tmp_dir_ = fs::temp_directory_path() / ("bagu-import-test-" +
                    std::to_string(::getpid()));
        fs::create_directories(tmp_dir_);
    }

    void TearDown() override {
        std::error_code ec;
        fs::remove_all(tmp_dir_, ec);
    }

    fs::path write_md(const std::string& filename, const std::string& content) {
        auto p = tmp_dir_ / filename;
        std::ofstream(p) << content;
        return p;
    }

    std::unique_ptr<db::Database> db_;
    std::unique_ptr<ImportService> svc_;
    fs::path tmp_dir_;
};

TEST_F(ImportServiceTest, ImportNewFile_CreatesTopicAndCards) {
    auto p = write_md("test.md",
        "# Title\n"
        "## 第 1 章 基础\n"
        "**Q1. 什么是 X？** Y 是答案\n"
        "## 第 11 章 题库\n"
        "**Q2. 第二题？** 第二答案\n");

    auto r = svc_->import_path(p);
    ASSERT_TRUE(r.is_ok()) << r.error().message;
    auto& summary = r.value();

    EXPECT_EQ(summary.total_files, 1);
    EXPECT_EQ(summary.succeeded, 1);
    EXPECT_EQ(summary.failed, 0);
    EXPECT_EQ(summary.skipped, 0);

    // 验证 db
    auto topic = db::TopicDao(*db_).find_by_name("test");
    ASSERT_TRUE(topic.is_ok());
    ASSERT_TRUE(topic.value().has_value());
    EXPECT_EQ(topic.value()->title, "Title");

    auto cards = db::CardDao(*db_).find_by_topic(topic.value()->id);
    ASSERT_TRUE(cards.is_ok());
    EXPECT_EQ(cards.value().size(), 2u);
}

TEST_F(ImportServiceTest, ImportTwice_SecondTimeIsSkipped) {
    auto p = write_md("a.md", "# T\n## 第 1 章 X\nbody\n");

    auto r1 = svc_->import_path(p);
    ASSERT_TRUE(r1.is_ok());
    EXPECT_EQ(r1.value().succeeded, 1);
    EXPECT_EQ(r1.value().skipped, 0);

    auto r2 = svc_->import_path(p);
    ASSERT_TRUE(r2.is_ok());
    EXPECT_EQ(r2.value().succeeded, 0);
    EXPECT_EQ(r2.value().skipped, 1);
}

TEST_F(ImportServiceTest, ContentChanged_TriggersUpdate) {
    auto p = write_md("a.md", "# T\n## 第 1 章\n旧内容\n");
    svc_->import_path(p);

    // 改内容
    write_md("a.md", "# T\n## 第 1 章\n新内容\n");

    auto r = svc_->import_path(p);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().succeeded, 1);
    EXPECT_TRUE(r.value().files[0].updated);

    // 校验 topic 还是 1 个，没有重复
    auto topics = db::TopicDao(*db_).find_all();
    EXPECT_EQ(topics.value().size(), 1u);
}

TEST_F(ImportServiceTest, ForceFlag_ReimportsEvenIfUnchanged) {
    auto p = write_md("a.md", "# T\nbody\n");
    svc_->import_path(p);

    ImportOptions opts;
    opts.force = true;
    auto r = svc_->import_path(p, opts);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().succeeded, 1);  // 强制视为成功
    EXPECT_EQ(r.value().skipped, 0);
    EXPECT_TRUE(r.value().files[0].updated);
}

TEST_F(ImportServiceTest, ImportDirectory_RecursiveScan) {
    write_md("one.md", "# A\n## 第 1 章\nbody\n");
    write_md("two.md", "# B\n## 第 1 章\nbody\n");

    // 子目录
    fs::create_directory(tmp_dir_ / "sub");
    std::ofstream(tmp_dir_ / "sub" / "three.md")
        << "# C\n## 第 1 章\nbody\n";

    auto r = svc_->import_path(tmp_dir_);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().total_files, 3);
    EXPECT_EQ(r.value().succeeded, 3);
}

TEST_F(ImportServiceTest, NonExistentPath_ReturnsError) {
    auto r = svc_->import_path("/tmp/this/does/not/exist");
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(E::kFileNotFound));
}

TEST_F(ImportServiceTest, EmptyDirectory_ReturnsError) {
    fs::create_directory(tmp_dir_ / "empty");
    auto r = svc_->import_path(tmp_dir_ / "empty");
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(E::kFileNotFound));
}

}  // namespace bagu::service
