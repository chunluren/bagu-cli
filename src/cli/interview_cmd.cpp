#include "cli/interview_cmd.h"

#include <toml++/toml.h>

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>

#include "bagu/error.h"
#include "db/database.h"
#include "db/migrations.h"
#include "db/topic_dao.h"
#include "llm/llm_client.h"
#include "service/interview_service.h"
#include "util/path.h"

namespace bagu::cli {

namespace {

// ANSI 颜色（仅 TTY）
struct Colors {
    bool enabled;
    const char* bold()    const { return enabled ? "\033[1m" : ""; }
    const char* dim()     const { return enabled ? "\033[2m" : ""; }
    const char* yellow()  const { return enabled ? "\033[33m" : ""; }
    const char* cyan()    const { return enabled ? "\033[36m" : ""; }
    const char* green()   const { return enabled ? "\033[32m" : ""; }
    const char* red()     const { return enabled ? "\033[31m" : ""; }
    const char* magenta() const { return enabled ? "\033[35m" : ""; }
    const char* reset()   const { return enabled ? "\033[0m" : ""; }
};

/// 加载 LLM 配置：从 ~/.bagu/config.toml + 环境变量 + CLI 选项
Result<llm::ClientConfig> load_llm_config(const InterviewCliOptions& opts) {
    llm::ClientConfig cfg;
    cfg.provider = opts.provider;
    cfg.model = opts.model;

    auto path = util::default_config_path();
    if (std::filesystem::exists(path)) {
        try {
            auto t = toml::parse_file(path.string());
            if (auto sec = t["llm"].as_table()) {
                if (cfg.provider.empty()) {
                    cfg.provider = sec->get_as<std::string>("provider")
                        ? **sec->get_as<std::string>("provider") : std::string("openai");
                }
                if (cfg.model.empty() && sec->get_as<std::string>("model")) {
                    cfg.model = **sec->get_as<std::string>("model");
                }
                if (sec->get_as<std::string>("base_url")) {
                    cfg.base_url = **sec->get_as<std::string>("base_url");
                }
                if (auto env_name = sec->get_as<std::string>("api_key_env")) {
                    const char* v = std::getenv((**env_name).c_str());
                    if (v) cfg.api_key = v;
                    if (!v && cfg.provider != "ollama") {
                        return make_err<llm::ClientConfig>(E::kLlmAuthFailed,
                            "环境变量未设置: " + std::string(**env_name),
                            "请 export " + std::string(**env_name) + "=<your-key>");
                    }
                }
            }
        } catch (const toml::parse_error& e) {
            return make_err<llm::ClientConfig>(E::kConfigInvalid,
                "config.toml 解析失败", std::string(e.description()));
        }
    }
    if (cfg.provider.empty()) cfg.provider = "openai";
    return cfg;
}

/// 用户多行输入：以单独一行 . 或 EOF (Ctrl+D) 结束
std::string read_user_answer() {
    std::ostringstream ss;
    std::string line;
    while (std::getline(std::cin, line)) {
        if (line == "." || line == ".\r") break;
        ss << line << "\n";
    }
    std::string s = ss.str();
    while (!s.empty() && (s.back() == '\n' || s.back() == ' ' || s.back() == '\r'))
        s.pop_back();
    return s;
}

}  // namespace

int run_interview(const InterviewCliOptions& opts) {
    if (opts.topic.empty()) {
        std::cerr << "Error: --topic 参数必填 (E"
                  << static_cast<int>(E::kArgRequired) << ")\n";
        return to_exit_code(static_cast<int>(E::kArgRequired));
    }

    // 1. 检查数据库
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

    // 应用最新 schema（含 v3 interview）
    db::register_all_migrations(db);
    auto m = db.migrate();
    if (m.is_err()) {
        std::cerr << "Error: 迁移失败: " << m.error().message << "\n";
        return to_exit_code(m.error().code);
    }

    // 2. 检查 topic
    auto t_r = db::TopicDao(db).find_by_name(opts.topic);
    if (t_r.is_err()) {
        std::cerr << "Error: " << t_r.error().message << "\n";
        return to_exit_code(t_r.error().code);
    }
    if (!t_r.value().has_value()) {
        std::cerr << "Error: 主题不存在: " << opts.topic << "\n";
        return to_exit_code(static_cast<int>(E::kTopicNotFound));
    }

    // 3. 加载 LLM 配置
    auto cfg_r = load_llm_config(opts);
    if (cfg_r.is_err()) {
        std::cerr << "Error: " << cfg_r.error().message << "\n  "
                  << cfg_r.error().detail << "\n";
        return to_exit_code(cfg_r.error().code);
    }
    auto& cfg = cfg_r.value();

    auto client_r = llm::make_client(cfg);
    if (client_r.is_err()) {
        std::cerr << "Error: " << client_r.error().message << "\n  "
                  << client_r.error().detail << "\n";
        return to_exit_code(client_r.error().code);
    }

    Colors C{::isatty(1) != 0};

    std::cout << "\n========== AI 模拟面试 ==========\n"
              << "主题: " << opts.topic << "\n"
              << "题数: " << opts.num << "\n"
              << "提供商: " << cfg.provider << "\n"
              << "模型: " << cfg.model << "\n\n"
              << C.dim() << "提示：每题答完后，单独一行输入 '.' 或按 Ctrl+D 结束本题。\n"
              << "输入 'q' 退出整个会话。\n" << C.reset() << "\n";

    service::InterviewService svc(db, std::move(client_r.value()));
    service::InterviewOptions iopts;
    iopts.topic = opts.topic;
    iopts.question_count = opts.num;
    auto sid_r = svc.start_session(iopts, cfg.model);
    if (sid_r.is_err()) {
        std::cerr << "Error: " << sid_r.error().message << "\n";
        return to_exit_code(sid_r.error().code);
    }
    int64_t session_id = sid_r.value();

    int answered = 0;
    int total_score = 0;
    std::string previous_summary;

    for (int q = 1; q <= opts.num; ++q) {
        std::cout << "\n" << C.bold() << C.cyan()
                  << "─── 第 " << q << "/" << opts.num << " 题 ───"
                  << C.reset() << "\n\n";

        // 1. 出题（流式输出）
        std::cout << C.yellow() << "[面试官] " << C.reset();
        std::string full_question;
        auto on_q_chunk = [&](const std::string& s) {
            std::cout << s << std::flush;
            full_question += s;
        };
        auto q_r = svc.next_question(q, previous_summary, on_q_chunk);
        std::cout << "\n";
        if (q_r.is_err()) {
            std::cerr << "\nError: 出题失败: " << q_r.error().message << "\n";
            break;
        }
        if (full_question.empty()) full_question = q_r.value();

        // 2. 用户答题
        std::cout << "\n" << C.green() << "[你]" << C.reset()
                  << " (多行答案，单独一行 . 或 Ctrl+D 结束；q 退出会话)\n";
        auto t0 = std::chrono::steady_clock::now();
        std::string user_answer = read_user_answer();
        auto t1 = std::chrono::steady_clock::now();
        int duration_ms = static_cast<int>(
            std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

        if (user_answer == "q" || user_answer == "Q") {
            std::cout << C.dim() << "(用户退出)\n" << C.reset();
            break;
        }

        // 3. 评分
        std::cout << "\n" << C.magenta() << "[评分] " << C.reset();
        std::string full_grade;
        auto on_g_chunk = [&](const std::string& s) {
            std::cout << s << std::flush;
            full_grade += s;
        };
        auto g_r = svc.grade_answer(full_question, user_answer, on_g_chunk);
        std::cout << "\n";
        if (g_r.is_err()) {
            std::cerr << "\nError: 评分失败: " << g_r.error().message << "\n";
            break;
        }
        if (g_r.value().feedback.empty()) {
            g_r.value().feedback = full_grade;
            g_r.value().score = service::InterviewService::parse_score(full_grade);
        }

        answered++;
        total_score += g_r.value().score;

        // 4. 保存
        auto sv = svc.save_qa(session_id, q, full_question, user_answer,
                              g_r.value(), duration_ms);
        if (sv.is_err()) {
            std::cerr << "warn: 保存失败: " << sv.error().message << "\n";
        }

        // 5. 准备下一题的摘要
        std::ostringstream sm;
        sm << "Q" << q << ": " << full_question.substr(0, 80)
           << "\n用户答得 " << g_r.value().score << "/10";
        previous_summary = sm.str();
    }

    double avg = answered > 0 ? static_cast<double>(total_score) / answered : 0.0;
    svc.end_session(session_id, avg, answered);

    std::cout << "\n========== 面试总结 ==========\n"
              << "完成题数: " << answered << " / " << opts.num << "\n";
    if (answered > 0) {
        std::cout << "平均分:   " << avg << " / 10\n"
                  << "总分:     " << total_score << "\n";
    }
    std::cout << "会话 ID:  " << session_id
              << " (可在数据库中回看)\n\n";
    return 0;
}

}  // namespace bagu::cli
