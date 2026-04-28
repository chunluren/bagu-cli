// bagu-cli — 八股文档智能学习助手
// 入口程序

#include <CLI/CLI.hpp>
#include <spdlog/spdlog.h>
#include <iostream>

#include "bagu/version.h"

int main(int argc, char** argv) {
    CLI::App app{"bagu - 八股文档智能学习助手"};
    app.set_version_flag("-v,--version",
        std::string("bagu ") + bagu::VERSION + " (built " + bagu::BUILD_DATE + ")");

    // ===== 子命令骨架（后续逐个实现）=====

    // bagu init
    auto* cmd_init = app.add_subcommand("init", "初始化数据目录与配置");

    // bagu import <path>
    auto* cmd_import = app.add_subcommand("import", "导入 markdown 八股文档");
    std::string import_path;
    cmd_import->add_option("path", import_path, "文档路径")->required();

    // bagu list [topic]
    auto* cmd_list = app.add_subcommand("list", "列出主题与章节");
    std::string list_topic;
    cmd_list->add_option("topic", list_topic, "主题名（可选）");

    // bagu search <keyword>
    auto* cmd_search = app.add_subcommand("search", "全文搜索");
    std::string search_keyword;
    cmd_search->add_option("keyword", search_keyword, "搜索关键词")->required();

    // bagu review
    auto* cmd_review = app.add_subcommand("review", "进入交互式复习模式");
    std::string review_topic;
    int review_num = 0;
    cmd_review->add_option("--topic", review_topic, "限定主题");
    cmd_review->add_option("-n,--num", review_num, "复习题数");

    // bagu interview
    auto* cmd_interview = app.add_subcommand("interview", "AI 模拟面试");
    std::string itv_topic;
    int itv_num = 5;
    cmd_interview->add_option("--topic", itv_topic, "主题")->required();
    cmd_interview->add_option("-n,--num", itv_num, "题数")->capture_default_str();

    // bagu stats
    auto* cmd_stats = app.add_subcommand("stats", "统计信息");
    bool stats_heatmap = false;
    cmd_stats->add_flag("--heatmap", stats_heatmap, "显示热力图");

    // bagu config
    auto* cmd_config = app.add_subcommand("config", "配置管理");
    cmd_config->require_subcommand(1);
    auto* cmd_config_get = cmd_config->add_subcommand("get", "获取配置");
    auto* cmd_config_set = cmd_config->add_subcommand("set", "设置配置");
    std::string cfg_key, cfg_value;
    cmd_config_get->add_option("key", cfg_key)->required();
    cmd_config_set->add_option("key", cfg_key)->required();
    cmd_config_set->add_option("value", cfg_value)->required();

    app.require_subcommand(1);

    CLI11_PARSE(app, argc, argv);

    // ===== 路由到具体处理函数 =====
    if (cmd_init->parsed()) {
        spdlog::info("TODO: 初始化数据目录");
        std::cout << "bagu init: not implemented yet\n";
    } else if (cmd_import->parsed()) {
        spdlog::info("TODO: 导入路径 {}", import_path);
        std::cout << "bagu import " << import_path << ": not implemented yet\n";
    } else if (cmd_list->parsed()) {
        spdlog::info("TODO: 列出 topic={}", list_topic);
        std::cout << "bagu list: not implemented yet\n";
    } else if (cmd_search->parsed()) {
        spdlog::info("TODO: 搜索 {}", search_keyword);
        std::cout << "bagu search '" << search_keyword << "': not implemented yet\n";
    } else if (cmd_review->parsed()) {
        spdlog::info("TODO: 复习 topic={} num={}", review_topic, review_num);
        std::cout << "bagu review: not implemented yet\n";
    } else if (cmd_interview->parsed()) {
        spdlog::info("TODO: 面试 topic={} num={}", itv_topic, itv_num);
        std::cout << "bagu interview: not implemented yet\n";
    } else if (cmd_stats->parsed()) {
        spdlog::info("TODO: 统计 heatmap={}", stats_heatmap);
        std::cout << "bagu stats: not implemented yet\n";
    } else if (cmd_config->parsed()) {
        if (cmd_config_get->parsed()) {
            std::cout << "bagu config get " << cfg_key << ": not implemented yet\n";
        } else {
            std::cout << "bagu config set " << cfg_key << " " << cfg_value
                      << ": not implemented yet\n";
        }
    }

    return 0;
}
