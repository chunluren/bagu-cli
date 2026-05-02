#include "http/http_server.h"

#include <gtest/gtest.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <thread>

#include "db/database.h"
#include "db/migrations.h"

namespace bagu::http {

using nlohmann::json;

class HttpServerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto r = db::Database::open(":memory:");
        ASSERT_TRUE(r.is_ok());
        db_ = std::make_unique<db::Database>(std::move(r.value()));
        db::register_all_migrations(*db_);
        ASSERT_TRUE(db_->migrate().is_ok());

        // 准备 topic + card
        db_->execute(
            "INSERT INTO topic (name, title, file_path, file_hash, imported_at, updated_at) "
            "VALUES ('mysql', 'MySQL', '/p', 'h', 0, 0)");
        db_->execute(
            "INSERT INTO card (topic_id, question, answer, created_at, updated_at) "
            "VALUES (1, 'MVCC?', '隐藏字段+undo+ReadView', 0, 0)");

        // 启动 server 在动态端口
        port_ = 18800 + (::getpid() % 1000);
        ServerOptions opts;
        opts.bind = "127.0.0.1";
        opts.port = port_;
        server_ = std::make_unique<HttpServer>(*db_, opts);

        // 后台线程跑
        thread_ = std::thread([this] { server_->run(); });

        // 等待 server 启动
        for (int i = 0; i < 50; ++i) {
            httplib::Client c("127.0.0.1", port_);
            c.set_connection_timeout(0, 100000);
            auto r = c.Get("/api/health");
            if (r && r->status == 200) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        FAIL() << "server did not start";
    }

    void TearDown() override {
        if (server_) server_->stop();
        if (thread_.joinable()) thread_.join();
    }

    httplib::Client client() { return httplib::Client("127.0.0.1", port_); }

    std::unique_ptr<db::Database> db_;
    std::unique_ptr<HttpServer> server_;
    std::thread thread_;
    int port_ = 0;
};

TEST_F(HttpServerTest, Health_ReturnsOk) {
    auto c = client();
    auto r = c.Get("/api/health");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 200);
    auto j = json::parse(r->body);
    EXPECT_EQ(j["ok"], true);
    EXPECT_EQ(j["schema_version"], 3);
}

TEST_F(HttpServerTest, Version_ReturnsString) {
    auto r = client().Get("/api/version");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 200);
    auto j = json::parse(r->body);
    EXPECT_TRUE(j.contains("version"));
    EXPECT_TRUE(j.contains("build_date"));
}

TEST_F(HttpServerTest, ListTopics_ReturnsArray) {
    auto r = client().Get("/api/topics");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 200);
    auto j = json::parse(r->body);
    EXPECT_TRUE(j.is_array());
    EXPECT_EQ(j.size(), 1u);
    EXPECT_EQ(j[0]["name"], "mysql");
    EXPECT_EQ(j[0]["cards"], 1);
}

TEST_F(HttpServerTest, GetTopic_ExistingReturnsDetails) {
    auto r = client().Get("/api/topics/mysql");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 200);
    auto j = json::parse(r->body);
    EXPECT_EQ(j["name"], "mysql");
    EXPECT_TRUE(j["chapters"].is_array());
}

TEST_F(HttpServerTest, GetTopic_Missing_Returns404WithErrorJson) {
    auto r = client().Get("/api/topics/notexist");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 404);
    auto j = json::parse(r->body);
    EXPECT_TRUE(j.contains("error"));
    EXPECT_EQ(j["error"]["code"], 4021);
}

TEST_F(HttpServerTest, GetCard_ExistingReturnsContent) {
    auto r = client().Get("/api/cards/1");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 200);
    auto j = json::parse(r->body);
    EXPECT_EQ(j["question"], "MVCC?");
}

TEST_F(HttpServerTest, GetCard_Missing_Returns404) {
    auto r = client().Get("/api/cards/99999");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 404);
    auto j = json::parse(r->body);
    EXPECT_EQ(j["error"]["code"], 4020);
}

TEST_F(HttpServerTest, GetCard_InvalidId_Returns400) {
    auto r = client().Get("/api/cards/abc");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 404);  // httplib regex 不匹配 → 404
}

TEST_F(HttpServerTest, Search_ReturnsItems) {
    auto r = client().Get("/api/search?q=MVCC");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 200);
    auto j = json::parse(r->body);
    EXPECT_EQ(j["query"], "MVCC");
    EXPECT_TRUE(j["items"].is_array());
    EXPECT_EQ(j["items"].size(), 1u);
}

TEST_F(HttpServerTest, Search_MissingQuery_Returns400) {
    auto r = client().Get("/api/search");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 400);
    auto j = json::parse(r->body);
    EXPECT_EQ(j["error"]["code"], 2000);
}

TEST_F(HttpServerTest, NotFound_ReturnsErrorJson) {
    auto r = client().Get("/api/no-such-route");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 404);
    auto j = json::parse(r->body);
    EXPECT_TRUE(j.contains("error"));
}

// ===== Review =====

TEST_F(HttpServerTest, ReviewDue_ReturnsNewCardsForFreshTopic) {
    auto r = client().Get("/api/review/due?topic=mysql&max_new=10");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 200);
    auto j = json::parse(r->body);
    ASSERT_TRUE(j["cards"].is_array());
    EXPECT_EQ(j["cards"].size(), 1u);  // 测试 fixture 里只有 1 张 card
    EXPECT_EQ(j["cards"][0]["is_new"], true);
}

TEST_F(HttpServerTest, ReviewGrade_UpdatesReview) {
    auto r = client().Post("/api/review/1/grade",
        R"({"score":5,"duration_ms":1234})", "application/json");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 200);
    auto j = json::parse(r->body);
    EXPECT_EQ(j["card_id"], 1);
    EXPECT_EQ(j["repetitions"], 1);
    EXPECT_EQ(j["interval_days"], 1);
    EXPECT_GT(j["ease_factor"].get<double>(), 2.5);
}

TEST_F(HttpServerTest, ReviewGrade_OutOfRange_Returns400) {
    auto r = client().Post("/api/review/1/grade",
        R"({"score":9})", "application/json");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 400);
    auto j = json::parse(r->body);
    EXPECT_EQ(j["error"]["code"], 2001);
}

TEST_F(HttpServerTest, ReviewGrade_BadJson_Returns400) {
    auto r = client().Post("/api/review/1/grade",
        "not-json", "application/json");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 400);
}

TEST_F(HttpServerTest, ReviewDue_AfterGrade_NoLongerDue) {
    // 给 card 1 打 5 分（间隔变为 1 天）
    client().Post("/api/review/1/grade",
        R"({"score":5,"duration_ms":0})", "application/json");

    // 取到期卡，max_new=0 → 应返回空
    auto r = client().Get("/api/review/due?topic=mysql&max_new=0");
    ASSERT_TRUE(r);
    auto j = json::parse(r->body);
    EXPECT_EQ(j["cards"].size(), 0u);
}

}  // namespace bagu::http
