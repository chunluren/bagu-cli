#include "llm/openai_client.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include "util/http.h"

namespace bagu::llm {

using nlohmann::json;

namespace {

json build_payload(const ChatRequest& req, const std::string& fallback_model, bool stream) {
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
    return body;
}

}  // namespace

OpenAIClient::OpenAIClient(std::string api_key, std::string base_url,
                           std::string default_model, int timeout_seconds)
    : api_key_(std::move(api_key)),
      base_url_(std::move(base_url)),
      default_model_(std::move(default_model)),
      timeout_seconds_(timeout_seconds) {
    while (!base_url_.empty() && base_url_.back() == '/') base_url_.pop_back();
}

Result<ChatResponse> OpenAIClient::chat(const ChatRequest& req) {
    std::string url = base_url_ + "/chat/completions";
    std::string body = build_payload(req, default_model_, false).dump();

    std::vector<std::string> headers = {
        "Content-Type: application/json",
    };
    if (!api_key_.empty()) {
        headers.push_back("Authorization: Bearer " + api_key_);
    }

    auto r = util::http_post_json(url, body, headers, timeout_seconds_);
    if (r.is_err()) return Result<ChatResponse>(r.error());

    auto& resp = r.value();
    if (resp.status_code == 401) {
        return make_err<ChatResponse>(E::kLlmAuthFailed,
            "LLM 认证失败（HTTP 401）", resp.body);
    }
    if (resp.status_code == 429) {
        return make_err<ChatResponse>(E::kLlmRateLimited,
            "LLM 限流（HTTP 429）", resp.body);
    }
    if (resp.status_code < 200 || resp.status_code >= 300) {
        return make_err<ChatResponse>(E::kHttpStatusError,
            "HTTP " + std::to_string(resp.status_code), resp.body);
    }

    try {
        json j = json::parse(resp.body);
        ChatResponse out;
        if (j.contains("choices") && !j["choices"].empty()) {
            auto& ch = j["choices"][0];
            if (ch.contains("message") && ch["message"].contains("content")) {
                out.content = ch["message"]["content"].get<std::string>();
            }
            if (ch.contains("finish_reason")) {
                out.finish_reason = ch["finish_reason"].get<std::string>();
            }
        }
        if (j.contains("usage")) {
            out.prompt_tokens = j["usage"].value("prompt_tokens", 0);
            out.completion_tokens = j["usage"].value("completion_tokens", 0);
        }
        return out;
    } catch (const json::exception& e) {
        return make_err<ChatResponse>(E::kLlmInvalidResponse,
            "JSON 解析失败", e.what());
    }
}

Result<ChatResponse> OpenAIClient::chat_stream(
    const ChatRequest& req,
    std::function<void(const std::string&)> on_chunk) {

    std::string url = base_url_ + "/chat/completions";
    std::string body = build_payload(req, default_model_, true).dump();

    std::vector<std::string> headers = {
        "Content-Type: application/json",
        "Accept: text/event-stream",
    };
    if (!api_key_.empty()) {
        headers.push_back("Authorization: Bearer " + api_key_);
    }

    ChatResponse out;
    bool error_seen = false;
    std::string error_detail;

    auto on_line = [&](const std::string& line) {
        // SSE 格式：每行 "data: {...}" 或空行
        if (line.empty()) return;
        constexpr const char* kPrefix = "data: ";
        if (line.compare(0, 6, kPrefix) != 0) return;
        std::string payload = line.substr(6);
        if (payload == "[DONE]") return;

        try {
            json j = json::parse(payload);
            if (j.contains("choices") && !j["choices"].empty()) {
                auto& ch = j["choices"][0];
                if (ch.contains("delta") && ch["delta"].contains("content")) {
                    auto& c = ch["delta"]["content"];
                    if (c.is_string()) {
                        std::string piece = c.get<std::string>();
                        out.content += piece;
                        if (on_chunk) on_chunk(piece);
                    }
                }
                if (ch.contains("finish_reason") && !ch["finish_reason"].is_null()) {
                    out.finish_reason = ch["finish_reason"].get<std::string>();
                }
            }
            if (j.contains("error")) {
                error_seen = true;
                error_detail = j["error"].dump();
            }
        } catch (const json::exception&) {
            // 忽略非 JSON 行（部分提供商可能在结束发其他元数据）
        }
    };

    auto r = util::http_post_json_stream(url, body, headers, on_line, timeout_seconds_);
    if (r.is_err()) return Result<ChatResponse>(r.error());

    int code = r.value();
    if (code == 401) return make_err<ChatResponse>(E::kLlmAuthFailed, "LLM 认证失败");
    if (code == 429) return make_err<ChatResponse>(E::kLlmRateLimited, "LLM 限流");
    if (code < 200 || code >= 300) {
        return make_err<ChatResponse>(E::kHttpStatusError,
            "HTTP " + std::to_string(code), error_detail);
    }
    if (error_seen) {
        return make_err<ChatResponse>(E::kLlmInvalidResponse,
            "LLM 返回错误", error_detail);
    }
    return out;
}

}  // namespace bagu::llm
