#include "cli/config_cmd.h"

#include <spdlog/spdlog.h>
#include <toml++/toml.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include "bagu/error.h"
#include "util/path.h"

namespace bagu::cli {

namespace fs = std::filesystem;

namespace {

/// 加载配置文件，失败返回 std::nullopt
std::optional<toml::table> load_config() {
    auto path = util::default_config_path();
    if (!fs::exists(path)) {
        std::cerr << "Error: 配置文件不存在 (E" << static_cast<int>(E::kConfigNotFound) << ")\n\n"
                  << "  path: " << path.string() << "\n\n"
                  << "提示：请先运行 `bagu init`\n";
        return std::nullopt;
    }

    try {
        return toml::parse_file(path.string());
    } catch (const toml::parse_error& e) {
        std::cerr << "Error: 配置文件解析失败 (E" << static_cast<int>(E::kConfigInvalid) << ")\n\n"
                  << "  " << e.description() << "\n";
        return std::nullopt;
    }
}

/// 把 "section.key" 拆成 (section, key)；无 section 返回 ("", key)
std::pair<std::string, std::string> split_key(const std::string& full_key) {
    auto pos = full_key.find('.');
    if (pos == std::string::npos) return {"", full_key};
    return {full_key.substr(0, pos), full_key.substr(pos + 1)};
}

/// 取值并打印
bool print_value(const toml::node& node) {
    if (auto v = node.value<std::string>()) {
        std::cout << *v << "\n";
        return true;
    }
    if (auto v = node.value<int64_t>()) {
        std::cout << *v << "\n";
        return true;
    }
    if (auto v = node.value<double>()) {
        std::cout << *v << "\n";
        return true;
    }
    if (auto v = node.value<bool>()) {
        std::cout << (*v ? "true" : "false") << "\n";
        return true;
    }
    return false;
}

}  // namespace

int run_config_get(const std::string& key) {
    auto cfg = load_config();
    if (!cfg) return to_exit_code(static_cast<int>(E::kConfigNotFound));

    auto [section, name] = split_key(key);

    const toml::node* node = nullptr;
    if (section.empty()) {
        node = cfg->get(name);
    } else {
        if (auto sec = cfg->get_as<toml::table>(section)) {
            node = sec->get(name);
        }
    }

    if (!node) {
        std::cerr << "Error: 配置项不存在: " << key << " (E"
                  << static_cast<int>(E::kConfigMissingField) << ")\n";
        return to_exit_code(static_cast<int>(E::kConfigMissingField));
    }

    if (!print_value(*node)) {
        std::cerr << "Error: 不支持的配置项类型: " << key << "\n";
        return to_exit_code(static_cast<int>(E::kConfigInvalid));
    }
    return 0;
}

int run_config_set(const std::string& key, const std::string& value) {
    auto cfg = load_config();
    if (!cfg) return to_exit_code(static_cast<int>(E::kConfigNotFound));

    auto [section, name] = split_key(key);

    toml::table* target = cfg.value().as_table();
    if (!section.empty()) {
        if (auto sec = cfg->get_as<toml::table>(section)) {
            target = sec;
        } else {
            cfg->insert(section, toml::table{});
            target = cfg->get_as<toml::table>(section);
        }
    }

    // 简单类型推断：纯数字 → int，true/false → bool，否则 → string
    if (value == "true") {
        target->insert_or_assign(name, true);
    } else if (value == "false") {
        target->insert_or_assign(name, false);
    } else {
        bool is_int = !value.empty();
        for (char c : value) {
            if (!std::isdigit(static_cast<unsigned char>(c)) && c != '-') {
                is_int = false;
                break;
            }
        }
        if (is_int) {
            try {
                target->insert_or_assign(name, std::stoll(value));
            } catch (...) {
                target->insert_or_assign(name, value);
            }
        } else {
            target->insert_or_assign(name, value);
        }
    }

    // 写回文件
    auto path = util::default_config_path();
    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        std::cerr << "Error: 无法写入配置文件 (E"
                  << static_cast<int>(E::kFileWriteFailed) << ")\n";
        return to_exit_code(static_cast<int>(E::kFileWriteFailed));
    }
    out << *cfg;

    std::cout << "Set " << key << " = " << value << "\n";
    return 0;
}

int run_config_list() {
    auto cfg = load_config();
    if (!cfg) return to_exit_code(static_cast<int>(E::kConfigNotFound));

    std::cout << *cfg;
    return 0;
}

}  // namespace bagu::cli
