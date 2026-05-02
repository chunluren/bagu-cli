#pragma once

#include <string>

#include "bagu/result.h"
#include "llm/llm_client.h"

namespace bagu::llm {

/// 用户传入的覆盖项
struct ConfigOverride {
    std::string provider;   // 空 = 用配置文件
    std::string model;      // 空 = 用配置文件
};

/// 从 ~/.bagu/config.toml + 环境变量加载 LLM 配置
///
/// 加载顺序：
///   1. CLI / 调用方覆盖（override）
///   2. config.toml `[llm]` 段
///   3. 环境变量（api_key 通过 `api_key_env` 间接读取）
///
/// 失败：
///   - kConfigInvalid：toml 解析失败
///   - kLlmAuthFailed：api_key_env 指向的变量未设置（ollama 除外）
Result<ClientConfig> load_config(const ConfigOverride& override);

}  // namespace bagu::llm
