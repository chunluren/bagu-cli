#pragma once

#include <string>
#include <vector>

#include "bagu/result.h"
#include "llm/llm_client.h"

namespace bagu::llm {

/// 用户传入的覆盖项
struct ConfigOverride {
    std::string provider;   // 空 = 用配置文件
    std::string model;      // 空 = 用配置文件
    std::string profile;    // 空 = 不用 profile；非空 → 取 [llm.profiles.<profile>]
};

/// 从 ~/.bagu/config.toml + 环境变量加载 LLM 配置
///
/// 解析顺序（后者覆盖前者）：
///   1. config.toml `[llm]` 段（默认值）
///   2. config.toml `[llm.profiles.<override.profile>]` 段（可选）
///   3. CLI / 调用方覆盖（override.provider / override.model）
///   4. 环境变量（api_key 通过 `api_key_env` 间接读取）
///
/// 失败：
///   - kConfigInvalid：toml 解析失败 / 指定 profile 不存在
///   - kLlmAuthFailed：api_key_env 指向的变量未设置（ollama 除外）
Result<ClientConfig> load_config(const ConfigOverride& override);

/// 列出 config.toml 里所有已定义的 profile 名（含隐式 "default"）
/// 用于 `bagu config list-profiles`
Result<std::vector<std::string>> list_profiles();

}  // namespace bagu::llm
