#include "llm/llm_client.h"
#include "llm/openai_client.h"

namespace bagu::llm {

Result<std::unique_ptr<LLMClient>> make_client(const ClientConfig& cfg) {
    if (cfg.provider == "openai" || cfg.provider == "openai-compat" || cfg.provider.empty()) {
        std::string base = cfg.base_url.empty() ? "https://api.openai.com/v1" : cfg.base_url;
        std::string model = cfg.model.empty() ? "gpt-4o-mini" : cfg.model;
        return std::unique_ptr<LLMClient>(
            new OpenAIClient(cfg.api_key, base, model, cfg.timeout_seconds));
    }

    if (cfg.provider == "ollama") {
        // Ollama 提供 OpenAI 兼容 API
        std::string base = cfg.base_url.empty() ? "http://localhost:11434/v1" : cfg.base_url;
        std::string model = cfg.model.empty() ? "llama3.2" : cfg.model;
        return std::unique_ptr<LLMClient>(
            new OpenAIClient(cfg.api_key, base, model, cfg.timeout_seconds));
    }

    if (cfg.provider == "claude") {
        // 暂用 OpenAI 兼容路径（很多 Claude 代理是 OpenAI 兼容）
        // TODO: 单独实现 Anthropic 原生 API
        std::string base = cfg.base_url.empty() ? "https://api.anthropic.com/v1" : cfg.base_url;
        std::string model = cfg.model.empty() ? "claude-sonnet-4-6" : cfg.model;
        return std::unique_ptr<LLMClient>(
            new OpenAIClient(cfg.api_key, base, model, cfg.timeout_seconds));
    }

    return make_err<std::unique_ptr<LLMClient>>(E::kLlmProviderUnknown,
        "未知的 LLM 提供商: " + cfg.provider,
        "支持: openai / openai-compat / claude / ollama");
}

}  // namespace bagu::llm
