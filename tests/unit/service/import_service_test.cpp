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

// ============================================================================
// stable_key / 重导入保留复习历史
// ============================================================================

namespace {
// 取一个 topic 下所有 card：(id, question, stable_key)
struct CardSnap { int64_t id; std::string question; std::string stable_key; };
std::vector<CardSnap> snapshot_cards(db::Database& db, int64_t topic_id) {
    std::vector<CardSnap> out;
    auto stmt = db.prepare(
        "SELECT id, question, IFNULL(stable_key, '') FROM card "
        "WHERE topic_id = ? ORDER BY id");
    stmt.bind(1, topic_id);
    while (stmt.step()) {
        CardSnap s;
        s.id = stmt.column_int64(0);
        s.question = stmt.column_text(1);
        s.stable_key = stmt.column_text(2);
        out.push_back(s);
    }
    return out;
}
}  // namespace

TEST_F(ImportServiceTest, FirstImport_AssignsStableKey) {
    auto p = write_md("st.md",
        "# ST\n## 第 1 章\n**Q1. 什么是 X？**\n是答案。\n");
    auto r = svc_->import_path(p);
    ASSERT_TRUE(r.is_ok());

    auto t_r = db::TopicDao(*db_).find_by_name("st");
    ASSERT_TRUE(t_r.is_ok() && t_r.value().has_value());
    auto cards = snapshot_cards(*db_, t_r.value()->id);
    ASSERT_EQ(cards.size(), 1u);
    EXPECT_EQ(cards[0].question, "什么是 X？");
    EXPECT_FALSE(cards[0].stable_key.empty());
    EXPECT_EQ(cards[0].stable_key.size(), 32u);   // sha256[:32]
}

TEST_F(ImportServiceTest, ReImport_SameQuestion_PreservesIdAndReviewHistory) {
    // 1. 第一次 import
    auto p = write_md("rt.md",
        "# RT\n## 第 1 章\n"
        "**Q1. 什么是 ACID？**\n原子隔离持久一致。\n"
        "**Q2. 什么是 MVCC？**\n多版本并发控制。\n");
    ASSERT_TRUE(svc_->import_path(p).is_ok());

    auto t_id = db::TopicDao(*db_).find_by_name("rt").value().value().id;
    auto before = snapshot_cards(*db_, t_id);
    ASSERT_EQ(before.size(), 2u);

    // 2. 模拟用户复习这两张卡
    for (auto& c : before) {
        auto stmt = db_->prepare(
            "INSERT INTO review (card_id, last_review, next_review, "
            " interval_days, ease_factor, repetitions, review_count, "
            " correct_count) VALUES (?, 1000, 86400, 1, 2.5, 1, 1, 1)");
        stmt.bind(1, c.id);
        ASSERT_GE(stmt.execute(), 0);

        auto h = db_->prepare(
            "INSERT INTO review_history (card_id, reviewed_at, score, duration_ms) "
            "VALUES (?, 1000, 4, 5000)");
        h.bind(1, c.id);
        ASSERT_GE(h.execute(), 0);
    }

    // 3. 改一下答案（题面不变）+ 加一张新卡，强制重导
    auto p2 = write_md("rt.md",
        "# RT\n## 第 1 章\n"
        "**Q1. 什么是 ACID？**\n原子隔离持久一致（更新版本）。\n"  // 答案改了
        "**Q2. 什么是 MVCC？**\n多版本并发控制。\n"               // 不变
        "**Q3. 什么是 Redo Log？**\n重做日志。\n");                // 新卡
    ImportService::Options opts;
    opts.force = true;
    auto r2 = svc_->import_path(p2, opts);
    ASSERT_TRUE(r2.is_ok());

    auto after = snapshot_cards(*db_, t_id);
    ASSERT_EQ(after.size(), 3u);

    // Q1, Q2 的 id 应被保留
    EXPECT_EQ(after[0].id, before[0].id) << "Q1 id changed";
    EXPECT_EQ(after[1].id, before[1].id) << "Q2 id changed";

    // Q1 答案应已更新
    {
        auto s = db_->prepare("SELECT answer FROM card WHERE id = ?");
        s.bind(1, before[0].id);
        ASSERT_TRUE(s.step());
        EXPECT_NE(s.column_text(0).find("更新版本"), std::string::npos);
    }

    // 复习历史保留：原两张卡的 review/review_history 应仍在
    for (auto& c : before) {
        auto rev = db_->prepare("SELECT COUNT(*) FROM review WHERE card_id = ?");
        rev.bind(1, c.id);
        ASSERT_TRUE(rev.step());
        EXPECT_EQ(rev.column_int(0), 1) << "review row lost for card " << c.id;

        auto h = db_->prepare(
            "SELECT COUNT(*) FROM review_history WHERE card_id = ?");
        h.bind(1, c.id);
        ASSERT_TRUE(h.step());
        EXPECT_EQ(h.column_int(0), 1) << "history lost for card " << c.id;
    }
}

TEST_F(ImportServiceTest, ReImport_DroppedQuestion_RemovesCardAndHistory) {
    // 第一次：3 张卡
    auto p = write_md("dr.md",
        "# DR\n## 第 1 章\n"
        "**Q1. A?**\nA 答\n"
        "**Q2. B?**\nB 答\n"
        "**Q3. C?**\nC 答\n");
    ASSERT_TRUE(svc_->import_path(p).is_ok());

    auto t_id = db::TopicDao(*db_).find_by_name("dr").value().value().id;
    auto before = snapshot_cards(*db_, t_id);
    ASSERT_EQ(before.size(), 3u);

    int64_t q3_id = before[2].id;
    auto h = db_->prepare(
        "INSERT INTO review_history (card_id, reviewed_at, score, duration_ms) "
        "VALUES (?, 1000, 5, 0)");
    h.bind(1, q3_id);
    ASSERT_GE(h.execute(), 0);

    // 重导入：Q3 没了
    auto p2 = write_md("dr.md",
        "# DR\n## 第 1 章\n"
        "**Q1. A?**\nA 答\n"
        "**Q2. B?**\nB 答\n");
    ImportService::Options opts; opts.force = true;
    ASSERT_TRUE(svc_->import_path(p2, opts).is_ok());

    auto after = snapshot_cards(*db_, t_id);
    ASSERT_EQ(after.size(), 2u);
    EXPECT_EQ(after[0].id, before[0].id);
    EXPECT_EQ(after[1].id, before[1].id);

    // Q3 卡 + 它的历史应通过 CASCADE 被删
    auto cnt = db_->prepare("SELECT COUNT(*) FROM card WHERE id = ?");
    cnt.bind(1, q3_id);
    ASSERT_TRUE(cnt.step());
    EXPECT_EQ(cnt.column_int(0), 0);

    auto hcnt = db_->prepare("SELECT COUNT(*) FROM review_history WHERE card_id = ?");
    hcnt.bind(1, q3_id);
    ASSERT_TRUE(hcnt.step());
    EXPECT_EQ(hcnt.column_int(0), 0);
}

TEST_F(ImportServiceTest, LegacyCards_NullStableKey_Backfilled_HistoryPreserved) {
    // 模拟 v3→v4 升级：先 import，再手工把 stable_key 清成 NULL
    auto p = write_md("lg.md",
        "# LG\n## 第 1 章\n"
        "**Q1. 为什么 epoll 比 select 快？**\n O(1) 触发回调。\n");
    ASSERT_TRUE(svc_->import_path(p).is_ok());

    auto t_id = db::TopicDao(*db_).find_by_name("lg").value().value().id;
    auto before = snapshot_cards(*db_, t_id);
    ASSERT_EQ(before.size(), 1u);
    int64_t old_id = before[0].id;

    // 写一条复习历史
    {
        auto h = db_->prepare(
            "INSERT INTO review_history (card_id, reviewed_at, score, duration_ms) "
            "VALUES (?, 1000, 5, 0)");
        h.bind(1, old_id);
        ASSERT_GE(h.execute(), 0);
    }
    // 把 stable_key 抹成 NULL，模拟 legacy
    {
        auto u = db_->prepare("UPDATE card SET stable_key = NULL WHERE id = ?");
        u.bind(1, old_id);
        ASSERT_GE(u.execute(), 0);
    }

    // 重导入相同内容（force）
    ImportService::Options opts; opts.force = true;
    ASSERT_TRUE(svc_->import_path(p, opts).is_ok());

    auto after = snapshot_cards(*db_, t_id);
    ASSERT_EQ(after.size(), 1u);
    EXPECT_EQ(after[0].id, old_id) << "legacy card.id should survive backfill+upsert";
    EXPECT_FALSE(after[0].stable_key.empty());

    // 复习历史应保留
    auto h = db_->prepare(
        "SELECT COUNT(*) FROM review_history WHERE card_id = ?");
    h.bind(1, old_id);
    ASSERT_TRUE(h.step());
    EXPECT_EQ(h.column_int(0), 1) << "legacy review history was wiped";
}

TEST_F(ImportServiceTest, NormalizeQuestion_WhitespaceInsensitive) {
    // 第一次：有空白
    auto p1 = write_md("nm.md",
        "# NM\n## 第 1 章\n"
        "**Q1.   什么是   ACID？**\n答 1\n");
    ASSERT_TRUE(svc_->import_path(p1).is_ok());
    auto t_id = db::TopicDao(*db_).find_by_name("nm").value().value().id;
    auto before = snapshot_cards(*db_, t_id);
    ASSERT_EQ(before.size(), 1u);

    // 第二次：空白不一样（外加换行），应被认为同一张
    auto p2 = write_md("nm.md",
        "# NM\n## 第 1 章\n"
        "**Q1. 什么是\tACID？**\n答 2\n");
    ImportService::Options opts; opts.force = true;
    ASSERT_TRUE(svc_->import_path(p2, opts).is_ok());

    auto after = snapshot_cards(*db_, t_id);
    ASSERT_EQ(after.size(), 1u);
    EXPECT_EQ(after[0].id, before[0].id) << "whitespace-only diff broke stable key";
}

}  // namespace bagu::service
