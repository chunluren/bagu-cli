#pragma once

#include <string>

#include "llm/llm_client.h"

namespace bagu::llm {

/// OpenAI 兼容客户端
/// （也可用于通义千问、Moonshot、DeepSeek、Ollama 等所有 OpenAI 兼容服务）
class OpenAIClient : public LLMClient {
public:
    OpenAIClient(std::string api_key,
                 std::string base_url = "https://api.openai.com/v1",
                 std::string default_model = "gpt-4o-mini",
                 int timeout_seconds = 60);

    Result<ChatResponse> chat(const ChatRequest& req) override;

    Result<ChatResponse> chat_stream(
        const ChatRequest& req,
        std::function<void(const std::string&)> on_chunk) override;

    std::string provider_name() const override { return "openai"; }

private:
    std::string api_key_;
    std::string base_url_;
    std::string default_model_;
    int timeout_seconds_;
};

}  // namespace bagu::llm
