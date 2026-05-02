#pragma once

#include <optional>
#include <string>

#include "llm/llm_client.h"

namespace bagu::llm {

/// OpenAI Chat Completions 协议的纯函数实现（无 IO，便于单测）
namespace openai_protocol {

/// 构建 /chat/completions 请求体 JSON 字符串
std::string build_chat_payload(const ChatRequest& req,
                               const std::string& fallback_model,
                               bool stream);

/// 解析非流式 chat 响应；失败返回错误码
Result<ChatResponse> parse_chat_response(const std::string& body);

/// 解析单行 SSE：返回增量内容 / finish_reason / 错误标记。
/// 任一字段为空表示该行没有该数据。
struct SseEvent {
    std::optional<std::string> delta_content;
    std::optional<std::string> finish_reason;
    bool is_done = false;             // line 是 "data: [DONE]"
    bool has_error = false;
    std::string error_detail;
};
SseEvent parse_sse_line(const std::string& line);

/// HTTP 状态码映射到 Error；2xx 返回 nullopt
std::optional<Error> map_http_status(int code, const std::string& body_or_detail);

/// 修剪 base_url 末尾的 `/`
std::string normalize_base_url(std::string url);

}  // namespace openai_protocol

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
