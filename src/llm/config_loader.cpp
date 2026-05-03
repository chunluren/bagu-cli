#include "llm/config_loader.h"

#include <toml++/toml.h>

#include <cstdlib>
#include <filesystem>

#include "bagu/error.h"
#include "util/path.h"

namespace bagu::llm {

namespace {

/// 从 toml table 提取字符串字段（不存在返回 nullopt）
std::optional<std::string> get_str(const toml::table* sec, std::string_view key) {
    if (!sec) return std::nullopt;
    if (auto* node = sec->get_as<std::string>(key)) return node->get();
    return std::nullopt;
}

/// 把 [llm] 或 [llm.profiles.<name>] 段的字段合并到 cfg。
/// override_only=true 时仅在字段为空时填（实现"profile 覆盖 default"语义）。
void merge_section(ClientConfig& cfg, const toml::table* sec, bool override_only) {
    if (!sec) return;
    if (auto v = get_str(sec, "provider")) {
        if (!override_only || cfg.provider.empty()) cfg.provider = *v;
    }
    if (auto v = get_str(sec, "model")) {
        if (!override_only || cfg.model.empty()) cfg.model = *v;
    }
    if (auto v = get_str(sec, "base_url")) {
        if (!override_only || cfg.base_url.empty()) cfg.base_url = *v;
    }
    // api_key_env 单独存到 cfg 里没字段，后面统一查 env
}

/// 从 [llm] / [llm.profiles.<name>] 解析 api_key_env：profile 优先
std::optional<std::string> resolve_api_key_env(const toml::table* default_sec,
                                              const toml::table* profile_sec) {
    if (auto v = get_str(profile_sec, "api_key_env")) return v;
    if (auto v = get_str(default_sec, "api_key_env")) return v;
    return std::nullopt;
}

}  // namespace

Result<ClientConfig> load_config(const ConfigOverride& ovr) {
    ClientConfig cfg;

    auto path = util::default_config_path();
    if (!std::filesystem::exists(path)) {
        // 没 config 文件 → 走 override / 默认
        if (!ovr.provider.empty()) cfg.provider = ovr.provider;
        if (!ovr.model.empty())    cfg.model    = ovr.model;
        if (cfg.provider.empty())  cfg.provider = "openai";
        return cfg;
    }

    toml::table t;
    try {
        t = toml::parse_file(path.string());
    } catch (const toml::parse_error& e) {
        return make_err<ClientConfig>(E::kConfigInvalid,
            "config.toml 解析失败", std::string(e.description()));
    }

    const toml::table* default_sec = t["llm"].as_table();
    const toml::table* profile_sec = nullptr;
    if (!ovr.profile.empty()) {
        profile_sec = t["llm"]["profiles"][ovr.profile].as_table();
        if (!profile_sec) {
            return make_err<ClientConfig>(E::kConfigInvalid,
                "profile 不存在: " + ovr.profile,
                "config.toml 缺 [llm.profiles." + ovr.profile + "] 段");
        }
    }

    // 优先级：override > profile > default
    // 1. 先把 default 全量写入空 cfg（force=true，因为现在 cfg 是空的）
    merge_section(cfg, default_sec, /*override_only=*/false);
    // 2. profile 覆盖（force=true）
    merge_section(cfg, profile_sec, /*override_only=*/false);
    // 3. CLI override 最后赢
    if (!ovr.provider.empty()) cfg.provider = ovr.provider;
    if (!ovr.model.empty())    cfg.model    = ovr.model;

    auto env_name = resolve_api_key_env(default_sec, profile_sec);
    if (env_name) {
        const char* v = std::getenv(env_name->c_str());
        if (v) cfg.api_key = v;
        if (!v && cfg.provider != "ollama") {
            return make_err<ClientConfig>(E::kLlmAuthFailed,
                "环境变量未设置: " + *env_name,
                "请 export " + *env_name + "=<your-key>");
        }
    }

    if (cfg.provider.empty()) cfg.provider = "openai";
    return cfg;
}

Result<std::vector<std::string>> list_profiles() {
    std::vector<std::string> out;
    auto path = util::default_config_path();
    if (!std::filesystem::exists(path)) {
        out.push_back("default");
        return out;
    }

    toml::table t;
    try {
        t = toml::parse_file(path.string());
    } catch (const toml::parse_error& e) {
        return make_err<std::vector<std::string>>(E::kConfigInvalid,
            "config.toml 解析失败", std::string(e.description()));
    }

    out.push_back("default");
    if (auto* profiles = t["llm"]["profiles"].as_table()) {
        for (const auto& [k, _] : *profiles) {
            out.emplace_back(std::string(k.str()));
        }
    }
    return out;
}

}  // namespace bagu::llm
