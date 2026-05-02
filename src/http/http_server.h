#pragma once

#include <httplib.h>

#include <string>

#include "db/database.h"

namespace bagu::http {

struct ServerOptions {
    std::string bind = "127.0.0.1";
    int port = 8780;
    std::string token;        // 非空时启用 Bearer 鉴权
    bool dev_mode = false;     // 开发模式：放宽 CORS
};

/// 嵌入式 HTTP server
///
/// @code
/// db::Database db = ...;
/// HttpServer server(db, opts);
/// server.run();    // 阻塞
/// @endcode
class HttpServer {
public:
    HttpServer(db::Database& db, ServerOptions opts);

    /// 启动监听（阻塞）
    /// @return 退出码（0 = 正常 / 非 0 = bind 失败等）
    int run();

    /// 停止 server（线程安全）
    void stop();

    /// 仅供测试：访问内部 svr_
    httplib::Server& server() { return svr_; }

private:
    void register_routes();
    void install_middleware();

    /// 各模块路由（拆开方便维护）
    void register_meta_routes();    // /api/health, /api/version
    void register_topic_routes();   // /api/topics, /api/topics/:name
    void register_card_routes();    // /api/cards, /api/cards/:id
    void register_search_routes();  // /api/search
    void register_review_routes();  // /api/review/due, /api/review/:id/grade
    void register_stats_routes();   // /api/stats/*

    db::Database& db_;
    ServerOptions opts_;
    httplib::Server svr_;
};

}  // namespace bagu::http
