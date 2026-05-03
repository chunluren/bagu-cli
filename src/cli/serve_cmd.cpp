#include "cli/serve_cmd.h"

#include <spdlog/spdlog.h>

#include <csignal>
#include <iostream>

#include "bagu/error.h"
#include "db/database.h"
#include "db/migrations.h"
#include "http/http_server.h"
#include "util/path.h"

namespace bagu::cli {

namespace {

http::HttpServer* g_server = nullptr;

void on_signal(int sig) {
    spdlog::info("收到信号 {}，停止 server...", sig);
    if (g_server) g_server->stop();
}

}  // namespace

int run_serve(const ServeOptions& opts) {
    auto db_path = util::default_db_path();
    if (!std::filesystem::exists(db_path)) {
        // 自动创建空数据库 + 应用 migrations。
        // Web UI 可在零数据状态下展示「请 import 题库」的引导页，
        // 比直接报错退出更友好（也方便 CI smoke 测）。
        spdlog::info("数据库不存在，将在 {} 创建空库", db_path.string());
        std::error_code ec;
        std::filesystem::create_directories(db_path.parent_path(), ec);
        if (ec) {
            std::cerr << "Error: 无法创建数据目录 " << db_path.parent_path()
                      << ": " << ec.message() << "\n";
            return to_exit_code(static_cast<int>(E::kFileNotFound));
        }
    }

    auto db_r = db::Database::open(db_path);
    if (db_r.is_err()) {
        std::cerr << "Error: " << db_r.error().message << "\n";
        return to_exit_code(db_r.error().code);
    }
    auto& db = db_r.value();

    db::register_all_migrations(db);
    auto m = db.migrate();
    if (m.is_err()) {
        std::cerr << "Error: " << m.error().message << "\n";
        return to_exit_code(m.error().code);
    }

    // serve 是常驻进程：info 级别更合适（启动 banner / access log）。
    // 已显式 --verbose 设成 debug 的话不动。
    if (spdlog::get_level() > spdlog::level::info) {
        spdlog::set_level(spdlog::level::info);
    }

    http::ServerOptions sopts;
    sopts.bind = opts.bind;
    sopts.port = opts.port;
    sopts.token = opts.token;
    sopts.dev_mode = opts.dev;

    http::HttpServer server(db, sopts);
    g_server = &server;

    std::signal(SIGINT, on_signal);
    std::signal(SIGTERM, on_signal);

    int rc = server.run();
    g_server = nullptr;
    return rc;
}

}  // namespace bagu::cli
