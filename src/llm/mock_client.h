#pragma once

#include <queue>

#include "llm/llm_client.h"

namespace bagu::llm {

/// 测试用 Mock 客户端
/// 使用：push 预设响应队列 → 每次 chat 弹出一个返回
class MockClient : public LLMClient {
public:
    /// 预设下一次 chat 的返回内容
    void push_response(const std::string& content) {
        responses_.push(content);
    }

    /// 取已发送的请求历史
    const std::vector<ChatRequest>& requests() const { return requests_; }

    Result<ChatResponse> chat(const ChatRequest& req) override {
        requests_.push_back(req);
        if (responses_.empty()) {
            return make_err<ChatResponse>(E::kInternal, "mock: no preset response");
        }
        ChatResponse r;
        r.content = responses_.front();
        r.prompt_tokens = 10;
        r.completion_tokens = 20;
        r.finish_reason = "stop";
        responses_.pop();
        return r;
    }

    Result<ChatResponse> chat_stream(
        const ChatRequest& req,
        std::function<void(const std::string&)> on_chunk) override {
        auto r = chat(req);
        if (r.is_ok() && on_chunk) on_chunk(r.value().content);
        return r;
    }

    std::string provider_name() const override { return "mock"; }

private:
    std::queue<std::string> responses_;
    std::vector<ChatRequest> requests_;
};

}  // namespace bagu::llm
