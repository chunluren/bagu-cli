// bagu-cli — 八股文档智能学习助手
// 入口程序

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include <iostream>

#include "bagu/version.h"
#include "cli/config_cmd.h"
#include "cli/due_cmd.h"
#include "cli/export_cmd.h"
#include "cli/import_cmd.h"
#include "cli/init_cmd.h"
#include "cli/interview_cmd.h"
#include "cli/list_cmd.h"
#include "cli/pause_cmd.h"
#include "cli/remind_cmd.h"
#include "cli/review_cmd.h"
#include "cli/search_cmd.h"
#include "cli/serve_cmd.h"
#include "cli/show_cmd.h"
#include "cli/stats_cmd.h"

int main(int argc, char** argv) {
    CLI::App app{"bagu - 八股文档智能学习助手"};
    app.set_version_flag("-v,--version",
        std::string("bagu ") + bagu::VERSION + " (built " + bagu::BUILD_DATE + ")");

    bool verbose = false;
    app.add_flag("--verbose", verbose, "详细日志");

    // ===== bagu init =====
    auto* cmd_init = app.add_subcommand("init", "初始化数据目录与配置");
    bool init_force = false;
    cmd_init->add_flag("--force", init_force, "已存在则覆盖");

    // ===== bagu import =====
    auto* cmd_import = app.add_subcommand("import", "导入 markdown 八股文档");
    std::string import_path;
    bool import_force = false;
    cmd_import->add_option("path", import_path, "文档路径")->required();
    cmd_import->add_flag("--force", import_force, "即使内容未变也重新导入");

    // ===== bagu list =====
    auto* cmd_list = app.add_subcommand("list", "列出主题与章节");
    std::string list_topic;
    cmd_list->add_option("topic", list_topic, "主题名（可选）");

    // ===== bagu show =====
    auto* cmd_show = app.add_subcommand("show", "查看单张卡片完整内容");
    int64_t show_id = 0;
    cmd_show->add_option("id", show_id, "卡片 ID")->required();

    // ===== bagu search =====
    auto* cmd_search = app.add_subcommand("search", "全文搜索");
    std::string search_keyword;
    std::string search_topic;
    int search_limit = 20;
    cmd_search->add_option("keyword", search_keyword, "搜索关键词")->required();
    cmd_search->add_option("--topic", search_topic, "限定主题");
    cmd_search->add_option("-n,--limit", search_limit, "返回上限")
        ->capture_default_str();

    // ===== bagu review =====
    auto* cmd_review = app.add_subcommand("review", "进入交互式复习模式");
    std::string review_topic;
    int review_num = 0;
    bool review_new_only = false;
    bool review_all = false;
    cmd_review->add_option("--topic", review_topic, "限定主题");
    cmd_review->add_option("-n,--num", review_num, "复习题数");
    cmd_review->add_flag("--new-only", review_new_only, "只学新卡");
    cmd_review->add_flag("--all", review_all, "复习所有到期卡片");

    // ===== bagu interview =====
    auto* cmd_interview = app.add_subcommand("interview", "AI 模拟面试");
    std::string itv_topic, itv_provider, itv_model, itv_profile;
    int itv_num = 5;
    cmd_interview->add_option("--topic", itv_topic, "主题")->required();
    cmd_interview->add_option("-n,--num", itv_num, "题数")->capture_default_str();
    cmd_interview->add_option("--profile", itv_profile,
        "用 ~/.bagu/config.toml 里的 [llm.profiles.<name>] 段");
    cmd_interview->add_option("--provider", itv_provider,
        "覆盖配置：openai / claude / ollama");
    cmd_interview->add_option("--model", itv_model, "覆盖配置：模型名");

    // ===== bagu stats =====
    auto* cmd_stats = app.add_subcommand("stats", "统计信息");
    std::string stats_topic;
    bool stats_heatmap = false;
    int stats_days = 90;
    cmd_stats->add_option("--topic", stats_topic, "限定主题");
    cmd_stats->add_flag("--heatmap", stats_heatmap, "显示热力图");
    cmd_stats->add_option("--days", stats_days, "热力图天数")->capture_default_str();

    // ===== bagu serve =====
    auto* cmd_serve = app.add_subcommand("serve", "启动本地 HTTP server（Web UI）");
    std::string serve_bind = "127.0.0.1";
    int serve_port = 8780;
    std::string serve_token;
    bool serve_dev = false;
    cmd_serve->add_option("--bind", serve_bind, "绑定地址")->capture_default_str();
    cmd_serve->add_option("--port,-p", serve_port, "端口")->capture_default_str();
    cmd_serve->add_option("--token", serve_token, "Bearer 鉴权 token");
    cmd_serve->add_flag("--dev", serve_dev, "开发模式（CORS 放宽）");

    // ===== bagu due =====
    auto* cmd_due = app.add_subcommand("due", "今日到期速览");
    std::string due_topic;
    cmd_due->add_option("--topic", due_topic, "限定主题");

    // ===== bagu pause / unpause =====
    auto* cmd_pause = app.add_subcommand("pause",
        "暂停指定卡片或整个主题的复习排程（review.suspended=1）");
    int64_t pause_card = 0;
    std::string pause_topic;
    cmd_pause->add_option("card_id", pause_card, "卡片 ID");
    cmd_pause->add_option("--topic", pause_topic, "暂停整个主题");

    auto* cmd_unpause = app.add_subcommand("unpause",
        "恢复指定卡片或主题的复习");
    int64_t unpause_card = 0;
    std::string unpause_topic;
    cmd_unpause->add_option("card_id", unpause_card, "卡片 ID");
    cmd_unpause->add_option("--topic", unpause_topic, "恢复整个主题");

    // ===== bagu remind =====
    auto* cmd_remind = app.add_subcommand("remind",
        "桌面通知今日到期（适合 cron / systemd timer / launchd）");
    std::string remind_topic;
    int remind_threshold = 1;
    bool remind_quiet = false, remind_dry = false;
    cmd_remind->add_option("--topic", remind_topic, "限定主题");
    cmd_remind->add_option("--threshold", remind_threshold,
        "到期数低于此值不通知")->capture_default_str();
    cmd_remind->add_flag("--quiet", remind_quiet, "不输出 stdout/stderr");
    cmd_remind->add_flag("--dry-run", remind_dry, "仅打印不发送");

    // ===== bagu export =====
    auto* cmd_export = app.add_subcommand("export",
        "导出题库（用法：bagu export anki [--topic T] [-o file]）");
    std::string export_format = "anki";
    std::string export_topic, export_output;
    bool export_include_section = false;
    cmd_export->add_option("format", export_format,
        "导出格式（目前仅 anki）")->capture_default_str();
    cmd_export->add_option("--topic", export_topic, "限定主题（默认全部）");
    cmd_export->add_option("-o,--output", export_output,
        "输出文件路径（默认 stdout）");
    cmd_export->add_flag("--include-section", export_include_section,
        "也导出章节占位卡（默认仅 qa 卡）");

    // ===== bagu config =====
    auto* cmd_config = app.add_subcommand("config", "配置管理");
    cmd_config->require_subcommand(1);
    auto* cmd_config_get = cmd_config->add_subcommand("get", "获取配置");
    auto* cmd_config_set = cmd_config->add_subcommand("set", "设置配置");
    auto* cmd_config_list = cmd_config->add_subcommand("list", "列出全部配置");
    auto* cmd_config_list_profiles = cmd_config->add_subcommand(
        "list-profiles", "列出可用的 LLM profiles");
    std::string cfg_get_key, cfg_set_key, cfg_set_value;
    cmd_config_get->add_option("key", cfg_get_key)->required();
    cmd_config_set->add_option("key", cfg_set_key)->required();
    cmd_config_set->add_option("value", cfg_set_value)->required();

    app.require_subcommand(1);

    CLI11_PARSE(app, argc, argv);

    if (verbose) {
        spdlog::set_level(spdlog::level::debug);
    } else {
        spdlog::set_level(spdlog::level::warn);
    }

    // ===== 路由 =====
    if (cmd_init->parsed()) {
        return bagu::cli::run_init(init_force);
    }
    if (cmd_config->parsed()) {
        if (cmd_config_get->parsed()) {
            return bagu::cli::run_config_get(cfg_get_key);
        }
        if (cmd_config_set->parsed()) {
            return bagu::cli::run_config_set(cfg_set_key, cfg_set_value);
        }
        if (cmd_config_list->parsed()) {
            return bagu::cli::run_config_list();
        }
        if (cmd_config_list_profiles->parsed()) {
            return bagu::cli::run_config_list_profiles();
        }
    }

    if (cmd_import->parsed()) {
        return bagu::cli::run_import(import_path, import_force);
    }
    // 以下命令尚未实现，占位
    if (cmd_list->parsed()) {
        return bagu::cli::run_list(list_topic);
    }
    if (cmd_show->parsed()) {
        return bagu::cli::run_show(show_id);
    }
    if (cmd_search->parsed()) {
        return bagu::cli::run_search(search_keyword, search_topic, search_limit);
    }
    if (cmd_review->parsed()) {
        bagu::cli::ReviewOptions ropts;
        ropts.topic = review_topic;
        ropts.num = review_num;
        ropts.new_only = review_new_only;
        ropts.all = review_all;
        return bagu::cli::run_review(ropts);
    }
    if (cmd_interview->parsed()) {
        bagu::cli::InterviewCliOptions iopts;
        iopts.topic = itv_topic;
        iopts.num = itv_num;
        iopts.provider = itv_provider;
        iopts.model = itv_model;
        iopts.profile = itv_profile;
        return bagu::cli::run_interview(iopts);
    }
    if (cmd_stats->parsed()) {
        bagu::cli::StatsOptions sopts;
        sopts.topic = stats_topic;
        sopts.heatmap = stats_heatmap;
        sopts.days = stats_days;
        return bagu::cli::run_stats(sopts);
    }
    if (cmd_serve->parsed()) {
        bagu::cli::ServeOptions sopts;
        sopts.bind = serve_bind;
        sopts.port = serve_port;
        sopts.token = serve_token;
        sopts.dev = serve_dev;
        return bagu::cli::run_serve(sopts);
    }
    if (cmd_due->parsed()) {
        bagu::cli::DueCliOptions dopts;
        dopts.topic = due_topic;
        return bagu::cli::run_due(dopts);
    }
    if (cmd_pause->parsed()) {
        bagu::cli::PauseCliOptions popts;
        popts.card_id = pause_card;
        popts.topic = pause_topic;
        popts.unpause = false;
        return bagu::cli::run_pause(popts);
    }
    if (cmd_unpause->parsed()) {
        bagu::cli::PauseCliOptions popts;
        popts.card_id = unpause_card;
        popts.topic = unpause_topic;
        popts.unpause = true;
        return bagu::cli::run_pause(popts);
    }
    if (cmd_remind->parsed()) {
        bagu::cli::RemindCliOptions ropts;
        ropts.topic = remind_topic;
        ropts.threshold = remind_threshold;
        ropts.quiet = remind_quiet;
        ropts.dry_run = remind_dry;
        return bagu::cli::run_remind(ropts);
    }
    if (cmd_export->parsed()) {
        bagu::cli::ExportCliOptions eopts;
        eopts.format = export_format;
        eopts.topic = export_topic;
        eopts.output = export_output;
        eopts.include_section_cards = export_include_section;
        return bagu::cli::run_export(eopts);
    }

    return 0;
}
