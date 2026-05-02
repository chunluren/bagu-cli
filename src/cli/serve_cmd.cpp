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
        std::cerr << "Error: 数据库不存在 (E"
                  << static_cast<int>(E::kDbNotFound) << ")\n\n"
                  << "提示：请先 `bagu init` 并 `bagu import <path>`\n";
        return to_exit_code(static_cast<int>(E::kDbNotFound));
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
