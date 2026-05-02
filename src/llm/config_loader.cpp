#include "llm/config_loader.h"

#include <toml++/toml.h>

#include <cstdlib>
#include <filesystem>

#include "bagu/error.h"
#include "util/path.h"

namespace bagu::llm {

Result<ClientConfig> load_config(const ConfigOverride& ovr) {
    ClientConfig cfg;
    cfg.provider = ovr.provider;
    cfg.model    = ovr.model;

    auto path = util::default_config_path();
    if (std::filesystem::exists(path)) {
        try {
            auto t = toml::parse_file(path.string());
            if (auto sec = t["llm"].as_table()) {
                auto str_at = [&](std::string_view k) -> std::optional<std::string> {
                    if (auto* node = sec->get_as<std::string>(k)) return node->get();
                    return std::nullopt;
                };

                if (cfg.provider.empty()) {
                    cfg.provider = str_at("provider").value_or("openai");
                }
                if (cfg.model.empty()) {
                    if (auto v = str_at("model")) cfg.model = *v;
                }
                if (auto v = str_at("base_url")) cfg.base_url = *v;

                if (auto env_name = str_at("api_key_env")) {
                    const char* v = std::getenv(env_name->c_str());
                    if (v) cfg.api_key = v;
                    if (!v && cfg.provider != "ollama") {
                        return make_err<ClientConfig>(E::kLlmAuthFailed,
                            "环境变量未设置: " + *env_name,
                            "请 export " + *env_name + "=<your-key>");
                    }
                }
            }
        } catch (const toml::parse_error& e) {
            return make_err<ClientConfig>(E::kConfigInvalid,
                "config.toml 解析失败", std::string(e.description()));
        }
    }
    if (cfg.provider.empty()) cfg.provider = "openai";
    return cfg;
}

}  // namespace bagu::llm
