#include "http/interview_routes.h"

#include <spdlog/spdlog.h>

#include <chrono>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <unordered_map>

#include "bagu/error.h"
#include "db/interview_dao.h"
#include "http/json_io.h"
#include "llm/config_loader.h"
#include "llm/llm_client.h"
#include "service/interview_service.h"

namespace bagu::http {

using nlohmann::json;

namespace {

// JSON 响应 helper
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

/// 把 InterviewSession 转 JSON
json to_json(const db::InterviewSession& s) {
    return {
        {"id", s.id},
        {"topic", s.topic},
        {"started_at", s.started_at},
        {"ended_at", s.ended_at},
        {"total_score", s.total_score},
        {"question_count", s.question_count},
        {"llm_provider", s.llm_provider},
        {"llm_model", s.llm_model},
    };
}

json to_json(const db::InterviewQA& q) {
    return {
        {"id", q.id},
        {"session_id", q.session_id},
        {"question_no", q.question_no},
        {"question", q.question},
        {"user_answer", q.user_answer},
        {"ai_score", q.ai_score},
        {"ai_feedback", q.ai_feedback},
        {"duration_ms", q.duration_ms},
    };
}

/// 转义 JSON 字符串中的特殊字符（用于 SSE 数据）
std::string sse_data(const json& payload) {
    // SSE：每行 "data: ..."；多行内容拆多个 data: 行（这里 JSON 单行即可）
    return "data: " + payload.dump() + "\n\n";
}

/// 进行中会话的 service 缓存（以 session_id 为 key）
///
/// 原因：next_question / grade_answer 都依赖 InterviewService 的成员变量
/// system_prompt_。如果每次 HTTP 请求重建 service，prompt 会丢失（CLI 流程
/// 是单进程内一直持有 service）。
///
/// 简化策略：内存中按 session_id 持有 service，会话 finish 时清除。进程
/// 重启会丢；这对单机本地工具足够。
class ServiceRegistry {
public:
    using Ptr = std::shared_ptr<service::InterviewService>;

    Ptr get(int64_t session_id) {
        std::lock_guard lk(mu_);
        auto it = map_.find(session_id);
        return it == map_.end() ? nullptr : it->second;
    }

    void set(int64_t session_id, Ptr p) {
        std::lock_guard lk(mu_);
        map_[session_id] = std::move(p);
    }

    void erase(int64_t session_id) {
        std::lock_guard lk(mu_);
        map_.erase(session_id);
    }

private:
    std::mutex mu_;
    std::unordered_map<int64_t, Ptr> map_;
};

/// session_id → 答题计数（用于 question_no）
class CounterRegistry {
public:
    int next(int64_t session_id) {
        std::lock_guard lk(mu_);
        return ++map_[session_id];
    }

    int current(int64_t session_id) {
        std::lock_guard lk(mu_);
        auto it = map_.find(session_id);
        return it == map_.end() ? 0 : it->second;
    }

    void erase(int64_t session_id) {
        std::lock_guard lk(mu_);
        map_.erase(session_id);
    }

private:
    std::mutex mu_;
    std::unordered_map<int64_t, int> map_;
};

/// session_id → 上一题摘要（出新题用）
class SummaryRegistry {
public:
    void set(int64_t session_id, std::string s) {
        std::lock_guard lk(mu_);
        map_[session_id] = std::move(s);
    }

    std::string get(int64_t session_id) {
        std::lock_guard lk(mu_);
        auto it = map_.find(session_id);
        return it == map_.end() ? std::string{} : it->second;
    }

    void erase(int64_t session_id) {
        std::lock_guard lk(mu_);
        map_.erase(session_id);
    }

private:
    std::mutex mu_;
    std::unordered_map<int64_t, std::string> map_;
};

/// session_id → 当前题目内容（评分时需要回传）
class QuestionRegistry {
public:
    void set(int64_t session_id, std::string s) {
        std::lock_guard lk(mu_);
        map_[session_id] = std::move(s);
    }

    std::string take(int64_t session_id) {
        std::lock_guard lk(mu_);
        auto it = map_.find(session_id);
        if (it == map_.end()) return {};
        std::string s = std::move(it->second);
        map_.erase(it);
        return s;
    }

    void erase(int64_t session_id) {
        std::lock_guard lk(mu_);
        map_.erase(session_id);
    }

private:
    std::mutex mu_;
    std::unordered_map<int64_t, std::string> map_;
};

/// session_id → 累计评分（avg 计算用）
class ScoreRegistry {
public:
    void add(int64_t session_id, int score) {
        std::lock_guard lk(mu_);
        auto& a = agg_[session_id];
        a.total += score;
        a.count += 1;
    }

    std::pair<int, int> snapshot(int64_t session_id) {
        std::lock_guard lk(mu_);
        auto it = agg_.find(session_id);
        if (it == agg_.end()) return {0, 0};
        return {it->second.total, it->second.count};
    }

    void erase(int64_t session_id) {
        std::lock_guard lk(mu_);
        agg_.erase(session_id);
    }

private:
    struct Agg { int total = 0; int count = 0; };
    std::mutex mu_;
    std::unordered_map<int64_t, Agg> agg_;
};

ServiceRegistry  g_services;
CounterRegistry  g_counters;
SummaryRegistry  g_summaries;
QuestionRegistry g_questions;
ScoreRegistry    g_scores;

void cleanup_session(int64_t id) {
    g_services.erase(id);
    g_counters.erase(id);
    g_summaries.erase(id);
    g_questions.erase(id);
    g_scores.erase(id);
}

}  // namespace

void register_interview_routes(httplib::Server& svr, db::Database& db) {
    // ============================================================
    // POST /api/interview/sessions
    // body: { "topic": "MySQL", "question_count": 5, "provider": "", "model": "" }
    // ============================================================
    svr.Post("/api/interview/sessions",
        [&db](const httplib::Request& req, httplib::Response& res) {
            json body;
            try {
                body = json::parse(req.body.empty() ? "{}" : req.body);
            } catch (const json::exception& e) {
                send_error(res, E::kArgInvalidValue, "invalid JSON body", e.what());
                return;
            }

            std::string topic = body.value("topic", "");
            if (topic.empty()) {
                send_error(res, E::kArgRequired, "topic 参数必填");
                return;
            }
            int qcount = body.value("question_count", 5);
            if (qcount < 1 || qcount > 20) {
                send_error(res, E::kArgInvalidValue,
                    "question_count 必须在 1-20 之间");
                return;
            }

            // 加载 LLM 配置
            llm::ConfigOverride ovr;
            ovr.provider = body.value("provider", "");
            ovr.model    = body.value("model", "");
            auto cfg_r = llm::load_config(ovr);
            if (cfg_r.is_err()) { send_error(res, cfg_r.error()); return; }
            auto& cfg = cfg_r.value();

            auto client_r = llm::make_client(cfg);
            if (client_r.is_err()) { send_error(res, client_r.error()); return; }

            auto svc = std::make_shared<service::InterviewService>(
                db, std::move(client_r.value()));

            service::InterviewOptions iopts;
            iopts.topic = topic;
            iopts.question_count = qcount;

            auto sid_r = svc->start_session(iopts, cfg.model);
            if (sid_r.is_err()) { send_error(res, sid_r.error()); return; }
            int64_t sid = sid_r.value();
            g_services.set(sid, svc);

            send_json(res, 200, json{
                {"session_id", sid},
                {"topic", topic},
                {"question_count", qcount},
                {"provider", cfg.provider},
                {"model", cfg.model},
            });
        });

    // ============================================================
    // GET /api/interview/sessions?limit=20  最近会话列表
    // ============================================================
    svr.Get("/api/interview/sessions",
        [&db](const httplib::Request& req, httplib::Response& res) {
            int limit = 20;
            if (req.has_param("limit")) {
                try { limit = std::stoi(req.get_param_value("limit")); }
                catch (...) { /* keep default */ }
            }
            if (limit < 1) limit = 1;
            if (limit > 200) limit = 200;

            auto r = db::InterviewDao(db).recent_sessions(limit);
            if (r.is_err()) { send_error(res, r.error()); return; }
            json arr = json::array();
            for (const auto& s : r.value()) arr.push_back(to_json(s));
            send_json(res, 200, json{{"items", arr}});
        });

    // ============================================================
    // GET /api/interview/sessions/:id  会话详情 + qa list
    // ============================================================
    svr.Get(R"(/api/interview/sessions/(\d+))",
        [&db](const httplib::Request& req, httplib::Response& res) {
            int64_t id = 0;
            try { id = std::stoll(req.matches[1].str()); }
            catch (...) {
                send_error(res, E::kArgInvalidValue, "invalid session id");
                return;
            }
            db::InterviewDao dao(db);
            auto s_r = dao.find_session(id);
            if (s_r.is_err()) { send_error(res, s_r.error()); return; }
            if (!s_r.value().has_value()) {
                send_error(res, E::kCardNotFound, "session not found",
                    "id=" + std::to_string(id));
                return;
            }
            auto qa_r = dao.qa_list(id);
            if (qa_r.is_err()) { send_error(res, qa_r.error()); return; }

            json out = to_json(*s_r.value());
            json arr = json::array();
            for (const auto& q : qa_r.value()) arr.push_back(to_json(q));
            out["qas"] = std::move(arr);
            send_json(res, 200, out);
        });

    // ============================================================
    // GET /api/interview/sessions/:id/question  SSE 出下一题
    // ============================================================
    svr.Get(R"(/api/interview/sessions/(\d+)/question)",
        [](const httplib::Request& req, httplib::Response& res) {
            int64_t sid = 0;
            try { sid = std::stoll(req.matches[1].str()); }
            catch (...) {
                send_error(res, E::kArgInvalidValue, "invalid session id");
                return;
            }
            auto svc = g_services.get(sid);
            if (!svc) {
                send_error(res, E::kCardNotFound,
                    "session 不在内存（请重新创建）",
                    "session_id=" + std::to_string(sid));
                return;
            }

            int q_no = g_counters.next(sid);
            std::string prev = g_summaries.get(sid);

            // 缓冲流式片段：cpp-httplib 的 chunked content provider 在不同
            // 线程调用 sink，但 LLM 客户端是同步阻塞的。这里把 LLM 调用放
            // chunk provider 内执行，把每个 chunk 直接 sink。
            res.set_chunked_content_provider(
                "text/event-stream; charset=utf-8",
                [svc, sid, q_no, prev](size_t, httplib::DataSink& sink) {
                    std::string full;
                    auto r = svc->next_question(q_no, prev,
                        [&](const std::string& s) {
                            full += s;
                            std::string chunk = sse_data(json{
                                {"type", "chunk"},
                                {"text", s},
                            });
                            sink.write(chunk.data(), chunk.size());
                        });
                    if (r.is_err()) {
                        std::string err = sse_data(json{
                            {"type", "error"},
                            {"message", r.error().message},
                            {"detail", r.error().detail},
                        });
                        sink.write(err.data(), err.size());
                    } else {
                        if (full.empty()) full = r.value();
                        // 缓存当前题目供评分用
                        g_questions.set(sid, full);
                        std::string done = sse_data(json{
                            {"type", "done"},
                            {"question_no", q_no},
                            {"content", full},
                        });
                        sink.write(done.data(), done.size());
                    }
                    sink.done();
                    return false;
                });
        });

    // ============================================================
    // POST /api/interview/sessions/:id/answer  SSE 评分
    // body: { "answer": "...", "duration_ms": 12345, "question": "(可选回退)" }
    // ============================================================
    svr.Post(R"(/api/interview/sessions/(\d+)/answer)",
        [](const httplib::Request& req, httplib::Response& res) {
            int64_t sid = 0;
            try { sid = std::stoll(req.matches[1].str()); }
            catch (...) {
                send_error(res, E::kArgInvalidValue, "invalid session id");
                return;
            }
            auto svc = g_services.get(sid);
            if (!svc) {
                send_error(res, E::kCardNotFound,
                    "session 不在内存（请重新创建）",
                    "session_id=" + std::to_string(sid));
                return;
            }

            json body;
            try {
                body = json::parse(req.body.empty() ? "{}" : req.body);
            } catch (const json::exception& e) {
                send_error(res, E::kArgInvalidValue, "invalid JSON body", e.what());
                return;
            }

            std::string answer = body.value("answer", "");
            if (answer.empty()) {
                send_error(res, E::kArgRequired, "answer 必填");
                return;
            }
            int duration_ms = body.value("duration_ms", 0);

            // 取出当前题目（前一次 question 接口缓存的；body 里也允许回退）
            std::string question = g_questions.take(sid);
            if (question.empty()) question = body.value("question", "");
            if (question.empty()) {
                send_error(res, E::kArgRequired,
                    "question 缺失（应先调 GET /question）");
                return;
            }

            int q_no = g_counters.current(sid);

            res.set_chunked_content_provider(
                "text/event-stream; charset=utf-8",
                [svc, sid, q_no, question, answer, duration_ms]
                (size_t, httplib::DataSink& sink) {
                    std::string full;
                    auto r = svc->grade_answer(question, answer,
                        [&](const std::string& s) {
                            full += s;
                            std::string chunk = sse_data(json{
                                {"type", "chunk"},
                                {"text", s},
                            });
                            sink.write(chunk.data(), chunk.size());
                        });
                    if (r.is_err()) {
                        std::string err = sse_data(json{
                            {"type", "error"},
                            {"message", r.error().message},
                            {"detail", r.error().detail},
                        });
                        sink.write(err.data(), err.size());
                        sink.done();
                        return false;
                    }

                    auto& g = r.value();
                    if (g.feedback.empty()) {
                        g.feedback = full;
                        g.score = service::InterviewService::parse_score(full);
                    }

                    auto sv = svc->save_qa(sid, q_no, question, answer, g, duration_ms);
                    int64_t qa_id = 0;  // dao.add_qa 返回 id 但 service 包了 void
                    (void)sv;

                    g_scores.add(sid, g.score);

                    // 写 summary 给下一题
                    std::string sm = "Q" + std::to_string(q_no) + ": "
                        + question.substr(0, 80)
                        + "\n用户答得 " + std::to_string(g.score) + "/10";
                    g_summaries.set(sid, sm);

                    std::string done = sse_data(json{
                        {"type", "done"},
                        {"question_no", q_no},
                        {"score", g.score},
                        {"feedback", g.feedback},
                        {"qa_id", qa_id},
                    });
                    sink.write(done.data(), done.size());
                    sink.done();
                    return false;
                });
        });

    // ============================================================
    // POST /api/interview/sessions/:id/finish  结束会话
    // ============================================================
    svr.Post(R"(/api/interview/sessions/(\d+)/finish)",
        [](const httplib::Request& req, httplib::Response& res) {
            int64_t sid = 0;
            try { sid = std::stoll(req.matches[1].str()); }
            catch (...) {
                send_error(res, E::kArgInvalidValue, "invalid session id");
                return;
            }
            auto svc = g_services.get(sid);
            if (!svc) {
                send_error(res, E::kCardNotFound,
                    "session 不在内存", "session_id=" + std::to_string(sid));
                return;
            }
            auto [total, count] = g_scores.snapshot(sid);
            double avg = count > 0 ? static_cast<double>(total) / count : 0.0;

            auto r = svc->end_session(sid, avg, count);
            if (r.is_err()) { send_error(res, r.error()); return; }

            cleanup_session(sid);
            send_json(res, 200, json{
                {"session_id", sid},
                {"answered", count},
                {"avg_score", avg},
                {"total_score", total},
            });
        });
}

}  // namespace bagu::http
