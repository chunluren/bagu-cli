#include <gtest/gtest.h>
#include <httplib.h>
#include <nlohmann/json.hpp>

#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <thread>

#include "db/database.h"
#include "db/migrations.h"
#include "http/http_server.h"

namespace bagu::http {

using nlohmann::json;
namespace fs = std::filesystem;

class InterviewRoutesTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 隔离 BAGU_HOME 到临时目录（避免读取真实 ~/.bagu/config.toml）
        tmp_home_ = fs::temp_directory_path() /
            ("bagu_iv_test_" + std::to_string(::getpid()));
        fs::create_directories(tmp_home_);
        ::setenv("BAGU_HOME", tmp_home_.string().c_str(), 1);

        auto r = db::Database::open(":memory:");
        ASSERT_TRUE(r.is_ok());
        db_ = std::make_unique<db::Database>(std::move(r.value()));
        db::register_all_migrations(*db_);
        ASSERT_TRUE(db_->migrate().is_ok());

        // 准备 topic
        db_->execute(
            "INSERT INTO topic (name, title, file_path, file_hash, "
            "imported_at, updated_at) "
            "VALUES ('mysql', 'MySQL', '/p', 'h', 0, 0)");

        port_ = 19800 + (::getpid() % 1000);
        ServerOptions opts;
        opts.bind = "127.0.0.1";
        opts.port = port_;
        server_ = std::make_unique<HttpServer>(*db_, opts);
        thread_ = std::thread([this] { server_->run(); });

        for (int i = 0; i < 50; ++i) {
            httplib::Client c("127.0.0.1", port_);
            c.set_connection_timeout(0, 100000);
            auto rr = c.Get("/api/health");
            if (rr && rr->status == 200) return;
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        FAIL() << "server did not start";
    }

    void TearDown() override {
        if (server_) server_->stop();
        if (thread_.joinable()) thread_.join();
        ::unsetenv("BAGU_HOME");
        std::error_code ec;
        fs::remove_all(tmp_home_, ec);
    }

    httplib::Client client() { return httplib::Client("127.0.0.1", port_); }

    fs::path tmp_home_;
    std::unique_ptr<db::Database> db_;
    std::unique_ptr<HttpServer> server_;
    std::thread thread_;
    int port_ = 0;
};

// ===== POST /api/interview/sessions 入参校验 =====

TEST_F(InterviewRoutesTest, CreateSession_InvalidJson_Returns400) {
    auto r = client().Post("/api/interview/sessions",
        "not json", "application/json");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 400);
    auto j = json::parse(r->body);
    EXPECT_TRUE(j.contains("error"));
}

TEST_F(InterviewRoutesTest, CreateSession_MissingTopic_Returns400) {
    auto r = client().Post("/api/interview/sessions",
        R"({"question_count": 5})", "application/json");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 400);
    auto j = json::parse(r->body);
    EXPECT_NE(j["error"]["message"].get<std::string>().find("topic"),
              std::string::npos);
}

TEST_F(InterviewRoutesTest, CreateSession_QuestionCountOutOfRange_Returns400) {
    auto r = client().Post("/api/interview/sessions",
        R"({"topic":"mysql","question_count": 99})",
        "application/json");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 400);
}

// ===== GET /api/interview/sessions 列最近会话 =====

TEST_F(InterviewRoutesTest, ListSessions_Empty_ReturnsEmptyArray) {
    auto r = client().Get("/api/interview/sessions");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 200);
    auto j = json::parse(r->body);
    ASSERT_TRUE(j.contains("items"));
    EXPECT_TRUE(j["items"].is_array());
    EXPECT_EQ(j["items"].size(), 0u);
}

TEST_F(InterviewRoutesTest, ListSessions_WithRecord_ReturnsItem) {
    db_->execute(
        "INSERT INTO interview_session "
        "(topic, started_at, ended_at, total_score, question_count, "
        " llm_provider, llm_model) "
        "VALUES ('mysql', 1000, 2000, 8.5, 3, 'mock', 'm')");

    auto r = client().Get("/api/interview/sessions");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 200);
    auto j = json::parse(r->body);
    ASSERT_EQ(j["items"].size(), 1u);
    EXPECT_EQ(j["items"][0]["topic"], "mysql");
    EXPECT_EQ(j["items"][0]["question_count"], 3);
}

// ===== GET /api/interview/sessions/:id 详情 =====

TEST_F(InterviewRoutesTest, GetSession_NotFound_Returns404) {
    auto r = client().Get("/api/interview/sessions/9999");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 404);
}

TEST_F(InterviewRoutesTest, GetSession_WithQA_ReturnsList) {
    db_->execute(
        "INSERT INTO interview_session "
        "(topic, started_at, question_count, llm_provider, llm_model) "
        "VALUES ('mysql', 1000, 0, 'mock', 'm')");
    db_->execute(
        "INSERT INTO interview_qa "
        "(session_id, question_no, question, user_answer, ai_score, "
        " ai_feedback, duration_ms) "
        "VALUES (1, 1, 'Q1?', 'ans', 7, '评分：7/10', 5000)");

    auto r = client().Get("/api/interview/sessions/1");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 200);
    auto j = json::parse(r->body);
    EXPECT_EQ(j["topic"], "mysql");
    ASSERT_EQ(j["qas"].size(), 1u);
    EXPECT_EQ(j["qas"][0]["ai_score"], 7);
}

// ===== /answer 在 session 不在内存时返回 404 =====

TEST_F(InterviewRoutesTest, Answer_SessionNotInMemory_Returns404) {
    auto r = client().Post("/api/interview/sessions/1/answer",
        R"({"answer":"x"})", "application/json");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 404);
}

TEST_F(InterviewRoutesTest, Finish_SessionNotInMemory_Returns404) {
    auto r = client().Post("/api/interview/sessions/1/finish",
        "{}", "application/json");
    ASSERT_TRUE(r);
    EXPECT_EQ(r->status, 404);
}

}  // namespace bagu::http
