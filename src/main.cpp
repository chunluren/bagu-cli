// bagu-cli — 八股文档智能学习助手
// 入口程序

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>

#include <iostream>

#include "bagu/version.h"
#include "cli/config_cmd.h"
#include "cli/import_cmd.h"
#include "cli/init_cmd.h"
#include "cli/list_cmd.h"
#include "cli/show_cmd.h"

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
    cmd_search->add_option("keyword", search_keyword, "搜索关键词")->required();

    // ===== bagu review =====
    auto* cmd_review = app.add_subcommand("review", "进入交互式复习模式");
    std::string review_topic;
    int review_num = 0;
    cmd_review->add_option("--topic", review_topic, "限定主题");
    cmd_review->add_option("-n,--num", review_num, "复习题数");

    // ===== bagu interview =====
    auto* cmd_interview = app.add_subcommand("interview", "AI 模拟面试");
    std::string itv_topic;
    int itv_num = 5;
    cmd_interview->add_option("--topic", itv_topic, "主题")->required();
    cmd_interview->add_option("-n,--num", itv_num, "题数")->capture_default_str();

    // ===== bagu stats =====
    auto* cmd_stats = app.add_subcommand("stats", "统计信息");
    bool stats_heatmap = false;
    cmd_stats->add_flag("--heatmap", stats_heatmap, "显示热力图");

    // ===== bagu config =====
    auto* cmd_config = app.add_subcommand("config", "配置管理");
    cmd_config->require_subcommand(1);
    auto* cmd_config_get = cmd_config->add_subcommand("get", "获取配置");
    auto* cmd_config_set = cmd_config->add_subcommand("set", "设置配置");
    auto* cmd_config_list = cmd_config->add_subcommand("list", "列出全部配置");
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
        std::cout << "bagu search '" << search_keyword << "': not implemented yet (M2)\n";
        return 0;
    }
    if (cmd_review->parsed()) {
        std::cout << "bagu review: not implemented yet (M3)\n";
        return 0;
    }
    if (cmd_interview->parsed()) {
        std::cout << "bagu interview: not implemented yet (M4)\n";
        return 0;
    }
    if (cmd_stats->parsed()) {
        std::cout << "bagu stats: not implemented yet (M4)\n";
        return 0;
    }

    return 0;
}
