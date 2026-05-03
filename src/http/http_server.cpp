#include "http/http_server.h"

#include <spdlog/spdlog.h>

#include <chrono>

#include "bagu/version.h"
#include "db/card_dao.h"
#include "db/chapter_dao.h"
#include "db/topic_dao.h"
#include "http/embedded_assets.h"
#include "http/json_io.h"
#include "service/export_service.h"
#include "service/review_service.h"
#include "service/stats_service.h"
#include "util/network.h"

#ifdef BAGU_HAVE_CURL
#include "http/interview_routes.h"
#include "llm/config_loader.h"
#endif

namespace bagu::http {

using nlohmann::json;

namespace {

// 写 JSON 响应辅助
void send_json(httplib::Response& res, int status, const json& j) {
    res.status = status;
    res.set_content(j.dump(), "application/json; charset=utf-8");
}

void send_error(httplib::Response& res, const Error& e) {
    send_json(res, error_to_http_status(e.code), error_to_json(e));
}

void send_error(httplib::Response& res, E code, const std::string& msg,
                const std::string& detail = {}) {
    send_error(res, Error{code, msg, detail});
}

}  // namespace

HttpServer::HttpServer(db::Database& db, ServerOptions opts)
    : db_(db), opts_(std::move(opts)) {}

int HttpServer::run() {
    install_middleware();
    register_routes();

    if (embedded_asset_count() > 0) {
        spdlog::info("Embedded Web UI: {} assets",
            embedded_asset_count());
    } else {
        spdlog::warn("Embedded Web UI: not bundled "
                     "(run 'cd web && npm run build' before cmake)");
    }
    spdlog::info("bagu serve listening on http://{}:{}", opts_.bind, opts_.port);
    if (opts_.bind == "0.0.0.0") {
        spdlog::warn("绑定到 0.0.0.0：同网络的其他设备可访问。"
                     "建议配合 --token 启用鉴权。");
        // 顺便列出本机所有 LAN IP，便于手机直接 copy URL
        auto ips = util::local_ipv4_addresses();
        if (!ips.empty()) {
            spdlog::info("可访问 URL（同 WiFi 内的其他设备）：");
            for (const auto& ip : ips) {
                spdlog::info("  http://{}:{}", ip, opts_.port);
            }
        }
    }
    if (!opts_.token.empty()) {
        spdlog::info("Bearer token 鉴权已启用");
    }

    if (!svr_.listen(opts_.bind.c_str(), opts_.port)) {
        spdlog::error("listen 失败: bind={} port={}（端口被占用？）",
            opts_.bind, opts_.port);
        return 1;
    }
    return 0;
}

void HttpServer::stop() {
    svr_.stop();
}

void HttpServer::install_middleware() {
    // ===== Pre-routing：access log + 鉴权 =====
    svr_.set_pre_routing_handler(
        [this](const httplib::Request& req, httplib::Response& res) {
            // 鉴权（非 /api/health 都校验）
            if (!opts_.token.empty() && req.path != "/api/health") {
                std::string auth = req.get_header_value("Authorization");
                std::string expected = "Bearer " + opts_.token;
                if (auth != expected) {
                    send_error(res, E::kLlmAuthFailed,
                        "未授权：缺失或错误的 Bearer token");
                    return httplib::Server::HandlerResponse::Handled;
                }
            }
            return httplib::Server::HandlerResponse::Unhandled;
        });

    // ===== Post-routing：access log =====
    svr_.set_logger([](const httplib::Request& req,
                       const httplib::Response& res) {
        spdlog::info("{} {} → {}", req.method, req.path, res.status);
    });

    // ===== 异常 handler =====
    svr_.set_exception_handler(
        [](const httplib::Request&, httplib::Response& res, std::exception_ptr ep) {
            std::string what = "unknown";
            try {
                if (ep) std::rethrow_exception(ep);
            } catch (const std::exception& e) {
                what = e.what();
            }
            spdlog::error("uncaught exception: {}", what);
            send_error(res, E::kInternal, "内部错误", what);
        });

    // ===== 404：API 路径 → JSON；其他路径 → 嵌入资源 fallback =====
    svr_.set_error_handler(
        [](const httplib::Request& req, httplib::Response& res) {
            if (res.status != 404 || !res.body.empty()) return;

            // API 路径 → JSON 错误
            if (req.path.rfind("/api/", 0) == 0) {
                json j = error_to_json(
                    Error{E::kNotImplemented, "Not Found", "path=" + req.path});
                res.set_content(j.dump(), "application/json; charset=utf-8");
                return;
            }

            // 静态资源 fallback：先精确匹配，否则 SPA → /index.html
            std::string path = req.path == "/" ? "/index.html" : req.path;
            const auto* asset = lookup_embedded_asset(path);
            if (!asset) {
                // SPA 路由（/topics/xxx, /search 等）→ 返回 index.html
                asset = lookup_embedded_asset("/index.html");
            }
            if (asset) {
                res.status = 200;
                res.set_content(
                    reinterpret_cast<const char*>(asset->data),
                    asset->size,
                    asset->mime);
                return;
            }

            // 没有嵌入资源（开发环境编译时未 build 前端）
            json j = error_to_json(
                Error{E::kNotImplemented, "Not Found",
                      "Web UI not bundled. Run 'cd web && npm run build' "
                      "and rebuild bagu, or use --dev mode."});
            res.set_content(j.dump(), "application/json; charset=utf-8");
        });

    // ===== CORS（仅 dev 模式放宽） =====
    if (opts_.dev_mode) {
        svr_.set_default_headers({
            {"Access-Control-Allow-Origin", "*"},
            {"Access-Control-Allow-Headers", "Content-Type, Authorization"},
            {"Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS"},
        });
        svr_.Options(R"(.*)",
            [](const httplib::Request&, httplib::Response& res) {
                res.status = 204;
            });
    }
}

void HttpServer::register_routes() {
    register_meta_routes();
    register_topic_routes();
    register_card_routes();
    register_search_routes();
    register_review_routes();
    register_stats_routes();
    register_export_routes();
#ifdef BAGU_HAVE_CURL
    register_interview_routes(svr_, db_);
#endif
}

// ============================================================================
// /api/health, /api/version
// ============================================================================
void HttpServer::register_meta_routes() {
    svr_.Get("/api/health", [this](const httplib::Request&, httplib::Response& res) {
        json j = {
            {"ok", true},
            {"version", bagu::VERSION},
            {"schema_version", db_.schema_version()},
        };
        send_json(res, 200, j);
    });

    svr_.Get("/api/version", [](const httplib::Request&, httplib::Response& res) {
        json j = {
            {"version", bagu::VERSION},
            {"build_date", bagu::BUILD_DATE},
        };
        send_json(res, 200, j);
    });

#ifdef BAGU_HAVE_CURL
    // LLM 配置文件里有哪些 profile 可用（v1.1+）
    svr_.Get("/api/llm/profiles",
        [](const httplib::Request&, httplib::Response& res) {
            auto r = llm::list_profiles();
            if (r.is_err()) { send_error(res, r.error()); return; }
            json arr = json::array();
            for (const auto& p : r.value()) arr.push_back(p);
            send_json(res, 200, json{{"profiles", arr}});
        });
#endif
}

// ============================================================================
// /api/topics
// ============================================================================
void HttpServer::register_topic_routes() {
    svr_.Get("/api/topics", [this](const httplib::Request&, httplib::Response& res) {
        db::TopicDao tdao(db_);
        auto r = tdao.find_all();
        if (r.is_err()) { send_error(res, r.error()); return; }

        db::CardDao cdao(db_);
        db::ChapterDao chdao(db_);

        json arr = json::array();
        for (const auto& t : r.value()) {
            json one = to_json(t);
            one["cards"]    = cdao.count_by_topic(t.id).value_or(0);
            one["chapters"] = chdao.count_by_topic(t.id).value_or(0);
            arr.push_back(std::move(one));
        }
        send_json(res, 200, arr);
    });

    svr_.Get(R"(/api/topics/([^/]+))",
        [this](const httplib::Request& req, httplib::Response& res) {
            std::string name = req.matches[1];
            db::TopicDao tdao(db_);
            auto t_r = tdao.find_by_name(name);
            if (t_r.is_err()) { send_error(res, t_r.error()); return; }
            if (!t_r.value().has_value()) {
                send_error(res, E::kTopicNotFound, "topic not found",
                    "name=" + name);
                return;
            }

            db::ChapterDao chdao(db_);
            auto chs = chdao.find_by_topic(t_r.value()->id);
            if (chs.is_err()) { send_error(res, chs.error()); return; }

            json out = to_json(*t_r.value());
            json arr = json::array();
            for (const auto& c : chs.value()) arr.push_back(to_json(c));
            out["chapters"] = std::move(arr);
            send_json(res, 200, out);
        });
}

// ============================================================================
// /api/cards
// ============================================================================
void HttpServer::register_card_routes() {
    svr_.Get(R"(/api/cards/(\d+))",
        [this](const httplib::Request& req, httplib::Response& res) {
            int64_t id = 0;
            try { id = std::stoll(req.matches[1].str()); }
            catch (...) {
                send_error(res, E::kArgInvalidValue, "invalid card id");
                return;
            }

            db::CardDao cdao(db_);
            auto r = cdao.find_by_id(id);
            if (r.is_err()) { send_error(res, r.error()); return; }
            if (!r.value().has_value()) {
                send_error(res, E::kCardNotFound,
                    "card not found", "id=" + std::to_string(id));
                return;
            }
            send_json(res, 200, to_json(*r.value()));
        });

    svr_.Get("/api/cards", [this](const httplib::Request& req, httplib::Response& res) {
        std::string topic = req.get_param_value("topic");
        if (topic.empty()) {
            send_error(res, E::kArgRequired,
                "topic 参数必填", "?topic=mysql");
            return;
        }
        db::TopicDao tdao(db_);
        auto t_r = tdao.find_by_name(topic);
        if (t_r.is_err()) { send_error(res, t_r.error()); return; }
        if (!t_r.value().has_value()) {
            send_error(res, E::kTopicNotFound, "topic not found");
            return;
        }
        db::CardDao cdao(db_);
        auto cards_r = cdao.find_by_topic(t_r.value()->id);
        if (cards_r.is_err()) { send_error(res, cards_r.error()); return; }

        json arr = json::array();
        for (const auto& c : cards_r.value()) arr.push_back(to_json(c));
        send_json(res, 200, json{
            {"topic", topic},
            {"items", arr},
            {"total", cards_r.value().size()},
        });
    });
}

// ============================================================================
// /api/review
// ============================================================================
void HttpServer::register_review_routes() {
    // GET /api/review/due?topic=xxx&max_due=20&max_new=5
    svr_.Get("/api/review/due",
        [this](const httplib::Request& req, httplib::Response& res) {
            service::ReviewPlan plan;
            plan.max_due = 20;
            plan.max_new = 5;

            std::string topic = req.get_param_value("topic");
            if (!topic.empty()) {
                auto t_r = db::TopicDao(db_).find_by_name(topic);
                if (t_r.is_err()) { send_error(res, t_r.error()); return; }
                if (!t_r.value().has_value()) {
                    send_error(res, E::kTopicNotFound, "topic not found");
                    return;
                }
                plan.topic_id = t_r.value()->id;
            }

            auto parse_int = [&](const std::string& key, int& out) {
                if (req.has_param(key)) {
                    try { out = std::stoi(req.get_param_value(key)); }
                    catch (...) { /* keep default */ }
                }
            };
            parse_int("max_due", plan.max_due);
            parse_int("max_new", plan.max_new);

            // 直接复用 ReviewService（虽然要新建一份 service 实例，DAO 是廉价的）
            service::ReviewService svc(db_);
            auto r = svc.get_due_cards(plan);
            if (r.is_err()) { send_error(res, r.error()); return; }

            json items = json::array();
            for (const auto& d : r.value()) items.push_back(to_json(d));
            send_json(res, 200, json{
                {"topic", topic},
                {"max_due", plan.max_due},
                {"max_new", plan.max_new},
                {"cards", items},
            });
        });

    // GET /api/review/due-summary — 各主题到期/新卡数 + 总数
    svr_.Get("/api/review/due-summary",
        [this](const httplib::Request&, httplib::Response& res) {
            auto r = service::ReviewService(db_).due_summary();
            if (r.is_err()) { send_error(res, r.error()); return; }
            const auto& s = r.value();
            json items = json::array();
            for (const auto& t : s.per_topic) {
                items.push_back({
                    {"topic_id", t.topic_id},
                    {"topic_name", t.topic_name},
                    {"topic_title", t.topic_title},
                    {"due", t.due},
                    {"new_cards", t.new_cards},
                });
            }
            send_json(res, 200, json{
                {"generated_at", s.generated_at},
                {"total_due", s.total_due},
                {"total_new", s.total_new},
                {"items", items},
            });
        });

    // POST /api/review/:id/grade  body: { "score": 4, "duration_ms": 5000 }
    svr_.Post(R"(/api/review/(\d+)/grade)",
        [this](const httplib::Request& req, httplib::Response& res) {
            int64_t card_id = 0;
            try { card_id = std::stoll(req.matches[1].str()); }
            catch (...) {
                send_error(res, E::kArgInvalidValue, "invalid card id");
                return;
            }

            int score = -1;
            int duration_ms = 0;
            try {
                json body = json::parse(req.body.empty() ? "{}" : req.body);
                if (body.contains("score") && body["score"].is_number_integer()) {
                    score = body["score"].get<int>();
                }
                if (body.contains("duration_ms") && body["duration_ms"].is_number_integer()) {
                    duration_ms = body["duration_ms"].get<int>();
                }
            } catch (const json::exception& e) {
                send_error(res, E::kArgInvalidValue, "invalid JSON body", e.what());
                return;
            }

            if (score < 0 || score > 5) {
                send_error(res, E::kArgInvalidValue,
                    "score 必须在 0-5 之间", "score=" + std::to_string(score));
                return;
            }

            service::ReviewService svc(db_);
            auto r = svc.submit_review(card_id, score, duration_ms);
            if (r.is_err()) { send_error(res, r.error()); return; }
            send_json(res, 200, to_json(r.value()));
        });
}

// ============================================================================
// /api/search
// ============================================================================
void HttpServer::register_search_routes() {
    svr_.Get("/api/search", [this](const httplib::Request& req, httplib::Response& res) {
        std::string q = req.get_param_value("q");
        if (q.empty()) {
            send_error(res, E::kArgRequired, "q 参数必填");
            return;
        }
        std::string topic = req.get_param_value("topic");
        int limit = 20;
        if (req.has_param("limit")) {
            try { limit = std::stoi(req.get_param_value("limit")); }
            catch (...) { /* keep default */ }
        }

        int64_t topic_id = 0;
        if (!topic.empty()) {
            auto t_r = db::TopicDao(db_).find_by_name(topic);
            if (t_r.is_err()) { send_error(res, t_r.error()); return; }
            if (!t_r.value().has_value()) {
                send_error(res, E::kTopicNotFound, "topic not found");
                return;
            }
            topic_id = t_r.value()->id;
        }

        auto t0 = std::chrono::steady_clock::now();
        auto r = db::CardDao(db_).search(q, topic_id, limit);
        auto t1 = std::chrono::steady_clock::now();
        int ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());
        if (r.is_err()) { send_error(res, r.error()); return; }

        json items = json::array();
        for (const auto& c : r.value()) items.push_back(to_json(c));

        send_json(res, 200, json{
            {"query", q},
            {"took_ms", ms},
            {"items", items},
        });
    });
}

// ============================================================================
// /api/stats
// ============================================================================
void HttpServer::register_stats_routes() {
    svr_.Get("/api/stats/overall",
        [this](const httplib::Request&, httplib::Response& res) {
            auto r = service::StatsService(db_).overall();
            if (r.is_err()) { send_error(res, r.error()); return; }
            const auto& s = r.value();
            send_json(res, 200, json{
                {"total_topics", s.total_topics},
                {"total_cards", s.total_cards},
                {"total_reviews", s.total_reviews},
                {"total_correct", s.total_correct},
                {"learned_unique_cards", s.learned_unique_cards},
                {"overall_accuracy", s.overall_accuracy},
                {"streak_days", s.streak_days},
                {"active_days_30", s.active_days_30},
            });
        });

    svr_.Get("/api/stats/topics",
        [this](const httplib::Request&, httplib::Response& res) {
            auto r = service::StatsService(db_).per_topic();
            if (r.is_err()) { send_error(res, r.error()); return; }
            json arr = json::array();
            for (const auto& t : r.value()) {
                arr.push_back({
                    {"topic_id", t.topic_id},
                    {"topic_name", t.topic_name},
                    {"title", t.title},
                    {"total_cards", t.total_cards},
                    {"learned_cards", t.learned_cards},
                    {"correct_cards", t.correct_cards},
                    {"accuracy", t.accuracy},
                    {"due_today", t.due_today},
                });
            }
            send_json(res, 200, json{{"items", arr}});
        });

    svr_.Get("/api/stats/heatmap",
        [this](const httplib::Request& req, httplib::Response& res) {
            int days = 90;
            if (req.has_param("days")) {
                try { days = std::stoi(req.get_param_value("days")); }
                catch (...) { /* keep default */ }
            }
            if (days < 1) days = 1;
            if (days > 365) days = 365;

            auto r = service::StatsService(db_).daily_counts(days);
            if (r.is_err()) { send_error(res, r.error()); return; }
            json arr = json::array();
            for (const auto& d : r.value()) {
                arr.push_back({{"date", d.date}, {"count", d.count}});
            }
            send_json(res, 200, json{{"days", days}, {"items", arr}});
        });

    svr_.Get("/api/stats/weak",
        [this](const httplib::Request& req, httplib::Response& res) {
            int recent_n = 200;
            int top_k = 10;
            if (req.has_param("recent")) {
                try { recent_n = std::stoi(req.get_param_value("recent")); }
                catch (...) { /* keep default */ }
            }
            if (req.has_param("top")) {
                try { top_k = std::stoi(req.get_param_value("top")); }
                catch (...) { /* keep default */ }
            }
            if (recent_n < 1) recent_n = 1;
            if (top_k < 1) top_k = 1;
            if (top_k > 100) top_k = 100;

            auto r = service::StatsService(db_).weak_cards(recent_n, top_k);
            if (r.is_err()) { send_error(res, r.error()); return; }
            json arr = json::array();
            for (const auto& w : r.value()) {
                arr.push_back({
                    {"card_id", w.card_id},
                    {"topic_name", w.topic_name},
                    {"question", w.question},
                    {"wrong_count", w.wrong_count},
                    {"total_recent", w.total_recent},
                });
            }
            send_json(res, 200, json{{"items", arr}});
        });
}

// ============================================================================
// /api/export/anki — 流式输出 Anki txt
// ============================================================================
void HttpServer::register_export_routes() {
    svr_.Get("/api/export/anki",
        [this](const httplib::Request& req, httplib::Response& res) {
            service::AnkiExportOptions opts;
            opts.topic = req.get_param_value("topic");
            opts.include_section_cards =
                req.get_param_value("include_section") == "1";

            // 先生成到 stringstream（cards 总数有限，内存可承）
            // 同时拿到 summary 写到响应头
            std::ostringstream buf;
            auto r = service::ExportService(db_).export_anki(opts, buf);
            if (r.is_err()) { send_error(res, r.error()); return; }

            res.status = 200;
            // 提供下载文件名
            std::string filename = "bagu-anki";
            if (!opts.topic.empty()) filename += "-" + opts.topic;
            filename += ".txt";
            res.set_header("Content-Disposition",
                "attachment; filename=\"" + filename + "\"");
            res.set_header("X-Bagu-Export-Total",
                std::to_string(r.value().total_cards));
            res.set_header("X-Bagu-Export-Written",
                std::to_string(r.value().written));
            res.set_content(buf.str(), "text/plain; charset=utf-8");
        });
}

}  // namespace bagu::http
