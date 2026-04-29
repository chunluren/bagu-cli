#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "bagu/result.h"

namespace bagu::llm {

/// 单条聊天消息
struct Message {
    std::string role;        // "system" / "user" / "assistant"
    std::string content;
};

/// 聊天请求
struct ChatRequest {
    std::vector<Message> messages;
    std::string model;             // 提供商特定，如 "gpt-4o-mini" / "claude-sonnet-4-6"
    double temperature = 0.7;
    int max_tokens = 1024;
    bool stream = false;
};

/// 聊天响应
struct ChatResponse {
    std::string content;          // 模型回复文本
    int prompt_tokens = 0;
    int completion_tokens = 0;
    std::string finish_reason;    // "stop" / "length" / ...
};

/// LLM 客户端抽象接口
///
/// 实现方：OpenAIClient / ClaudeClient / OllamaClient / MockClient
class LLMClient {
public:
    virtual ~LLMClient() = default;

    /// 同步调用（一次返回完整结果）
    virtual Result<ChatResponse> chat(const ChatRequest& req) = 0;

    /// 流式调用（每收到一段文本调用 on_chunk）
    /// @param on_chunk 增量文本回调（不含历史，仅本次新增）
    /// @return 完整响应（同时回调已经把片段都发完了）
    virtual Result<ChatResponse> chat_stream(
        const ChatRequest& req,
        std::function<void(const std::string& chunk)> on_chunk) = 0;

    /// 提供商名（"openai" / "claude" / "ollama" / "mock"）
    virtual std::string provider_name() const = 0;
};

/// 客户端工厂参数
struct ClientConfig {
    std::string provider;       // "openai" / "claude" / "ollama" / "mock"
    std::string api_key;         // 直接传 key（生产从环境变量读后传入）
    std::string model;           // 默认模型
    std::string base_url;        // 自定义 endpoint（OpenAI 兼容服务 / Ollama）
    int timeout_seconds = 60;
};

/// 工厂函数：根据配置创建客户端
Result<std::unique_ptr<LLMClient>> make_client(const ClientConfig& cfg);

}  // namespace bagu::llm
