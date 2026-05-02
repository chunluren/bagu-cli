#include "llm/openai_client.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "util/http.h"

namespace bagu::llm {

using nlohmann::json;

// ============================================================================
// 协议 helpers（纯函数，无 IO，便于单测）
// ============================================================================
namespace openai_protocol {

std::string build_chat_payload(const ChatRequest& req,
                               const std::string& fallback_model,
                               bool stream) {
    json messages = json::array();
    for (const auto& m : req.messages) {
        messages.push_back({{"role", m.role}, {"content", m.content}});
    }
    json body = {
        {"model", req.model.empty() ? fallback_model : req.model},
        {"messages", messages},
        {"temperature", req.temperature},
        {"max_tokens", req.max_tokens},
        {"stream", stream},
    };
    return body.dump();
}

Result<ChatResponse> parse_chat_response(const std::string& body) {
    try {
        json j = json::parse(body);
        ChatResponse out;
        if (j.contains("choices") && !j["choices"].empty()) {
            auto& ch = j["choices"][0];
            if (ch.contains("message") && ch["message"].contains("content") &&
                ch["message"]["content"].is_string()) {
                out.content = ch["message"]["content"].get<std::string>();
            }
            if (ch.contains("finish_reason") && ch["finish_reason"].is_string()) {
                out.finish_reason = ch["finish_reason"].get<std::string>();
            }
        }
        if (j.contains("usage")) {
            out.prompt_tokens     = j["usage"].value("prompt_tokens", 0);
            out.completion_tokens = j["usage"].value("completion_tokens", 0);
        }
        return out;
    } catch (const json::exception& e) {
        return make_err<ChatResponse>(E::kLlmInvalidResponse,
            "JSON 解析失败", e.what());
    }
}

SseEvent parse_sse_line(const std::string& line) {
    SseEvent ev;
    if (line.empty()) return ev;
    constexpr const char* kPrefix = "data: ";
    if (line.compare(0, 6, kPrefix) != 0) return ev;
    std::string payload = line.substr(6);
    if (payload == "[DONE]") {
        ev.is_done = true;
        return ev;
    }
    try {
        json j = json::parse(payload);
        if (j.contains("choices") && !j["choices"].empty()) {
            auto& ch = j["choices"][0];
            if (ch.contains("delta") && ch["delta"].contains("content")) {
                auto& c = ch["delta"]["content"];
                if (c.is_string()) ev.delta_content = c.get<std::string>();
            }
            if (ch.contains("finish_reason") && ch["finish_reason"].is_string()) {
                ev.finish_reason = ch["finish_reason"].get<std::string>();
            }
        }
        if (j.contains("error")) {
            ev.has_error = true;
            ev.error_detail = j["error"].dump();
        }
    } catch (const json::exception&) {
        // 非 JSON 行（部分提供商可能在结束发其他元数据）→ 静默忽略
    }
    return ev;
}

std::optional<Error> map_http_status(int code, const std::string& body) {
    if (code == 401) return Error{E::kLlmAuthFailed, "LLM 认证失败（HTTP 401）", body};
    if (code == 429) return Error{E::kLlmRateLimited, "LLM 限流（HTTP 429）", body};
    if (code < 200 || code >= 300) {
        return Error{E::kHttpStatusError, "HTTP " + std::to_string(code), body};
    }
    return std::nullopt;
}

std::string normalize_base_url(std::string url) {
    while (!url.empty() && url.back() == '/') url.pop_back();
    return url;
}

}  // namespace openai_protocol

// ============================================================================
// OpenAIClient
// ============================================================================
OpenAIClient::OpenAIClient(std::string api_key, std::string base_url,
                           std::string default_model, int timeout_seconds)
    : api_key_(std::move(api_key)),
      base_url_(openai_protocol::normalize_base_url(std::move(base_url))),
      default_model_(std::move(default_model)),
      timeout_seconds_(timeout_seconds) {}

Result<ChatResponse> OpenAIClient::chat(const ChatRequest& req) {
    std::string url  = base_url_ + "/chat/completions";
    std::string body = openai_protocol::build_chat_payload(req, default_model_, false);

    std::vector<std::string> headers = {"Content-Type: application/json"};
    if (!api_key_.empty()) headers.push_back("Authorization: Bearer " + api_key_);

    auto r = util::http_post_json(url, body, headers, timeout_seconds_);
    if (r.is_err()) return Result<ChatResponse>(r.error());

    auto& resp = r.value();
    if (auto e = openai_protocol::map_http_status(resp.status_code, resp.body); e) {
        return Result<ChatResponse>(*e);
    }
    return openai_protocol::parse_chat_response(resp.body);
}

Result<ChatResponse> OpenAIClient::chat_stream(
    const ChatRequest& req,
    std::function<void(const std::string&)> on_chunk) {

    std::string url  = base_url_ + "/chat/completions";
    std::string body = openai_protocol::build_chat_payload(req, default_model_, true);

    std::vector<std::string> headers = {
        "Content-Type: application/json",
        "Accept: text/event-stream",
    };
    if (!api_key_.empty()) headers.push_back("Authorization: Bearer " + api_key_);

    ChatResponse out;
    bool error_seen = false;
    std::string error_detail;

    auto on_line = [&](const std::string& line) {
        auto ev = openai_protocol::parse_sse_line(line);
        if (ev.is_done) return;
        if (ev.delta_content) {
            out.content += *ev.delta_content;
            if (on_chunk) on_chunk(*ev.delta_content);
        }
        if (ev.finish_reason) out.finish_reason = *ev.finish_reason;
        if (ev.has_error) {
            error_seen   = true;
            error_detail = ev.error_detail;
        }
    };

    auto r = util::http_post_json_stream(url, body, headers, on_line, timeout_seconds_);
    if (r.is_err()) return Result<ChatResponse>(r.error());

    if (auto e = openai_protocol::map_http_status(r.value(), error_detail); e) {
        return Result<ChatResponse>(*e);
    }
    if (error_seen) {
        return make_err<ChatResponse>(E::kLlmInvalidResponse,
            "LLM 返回错误", error_detail);
    }
    return out;
}

}  // namespace bagu::llm
