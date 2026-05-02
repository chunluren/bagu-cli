#include "llm/openai_client.h"

#include <gtest/gtest.h>
#include <nlohmann/json.hpp>

namespace bagu::llm::openai_protocol {

using nlohmann::json;

// ===== build_chat_payload =====

TEST(OpenAIProtocol_BuildPayload, IncludesAllFields) {
    ChatRequest req;
    req.messages = {{"system", "you are helpful"}, {"user", "hi"}};
    req.temperature = 0.42;
    req.max_tokens = 500;

    auto j = json::parse(build_chat_payload(req, "gpt-4o-mini", false));
    EXPECT_EQ(j["model"], "gpt-4o-mini");
    EXPECT_EQ(j["temperature"], 0.42);
    EXPECT_EQ(j["max_tokens"], 500);
    EXPECT_EQ(j["stream"], false);
    ASSERT_EQ(j["messages"].size(), 2u);
    EXPECT_EQ(j["messages"][0]["role"], "system");
    EXPECT_EQ(j["messages"][1]["content"], "hi");
}

TEST(OpenAIProtocol_BuildPayload, ReqModelOverridesFallback) {
    ChatRequest req;
    req.model = "gpt-4o";
    req.messages = {{"user", "x"}};
    auto j = json::parse(build_chat_payload(req, "fallback", false));
    EXPECT_EQ(j["model"], "gpt-4o");
}

TEST(OpenAIProtocol_BuildPayload, EmptyReqModelUsesFallback) {
    ChatRequest req;
    req.messages = {{"user", "x"}};
    auto j = json::parse(build_chat_payload(req, "fallback-model", false));
    EXPECT_EQ(j["model"], "fallback-model");
}

TEST(OpenAIProtocol_BuildPayload, StreamFlagPropagated) {
    ChatRequest req;
    auto j = json::parse(build_chat_payload(req, "m", true));
    EXPECT_EQ(j["stream"], true);
}

// ===== parse_chat_response =====

TEST(OpenAIProtocol_ParseResponse, NormalResponse) {
    std::string body = R"({
        "choices": [{
            "message": {"role": "assistant", "content": "hello world"},
            "finish_reason": "stop"
        }],
        "usage": {"prompt_tokens": 10, "completion_tokens": 20}
    })";
    auto r = parse_chat_response(body);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().content, "hello world");
    EXPECT_EQ(r.value().finish_reason, "stop");
    EXPECT_EQ(r.value().prompt_tokens, 10);
    EXPECT_EQ(r.value().completion_tokens, 20);
}

TEST(OpenAIProtocol_ParseResponse, MissingChoicesReturnsEmptyContent) {
    auto r = parse_chat_response("{}");
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().content, "");
}

TEST(OpenAIProtocol_ParseResponse, InvalidJsonReturnsError) {
    auto r = parse_chat_response("not json {");
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(E::kLlmInvalidResponse));
}

TEST(OpenAIProtocol_ParseResponse, MissingUsageDefaultsToZero) {
    auto r = parse_chat_response(
        R"({"choices":[{"message":{"content":"x"}}]})");
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().prompt_tokens, 0);
    EXPECT_EQ(r.value().completion_tokens, 0);
}

// ===== parse_sse_line =====

TEST(OpenAIProtocol_ParseSse, ContentDelta) {
    auto ev = parse_sse_line(
        R"(data: {"choices":[{"delta":{"content":"hello"}}]})");
    ASSERT_TRUE(ev.delta_content.has_value());
    EXPECT_EQ(*ev.delta_content, "hello");
    EXPECT_FALSE(ev.is_done);
    EXPECT_FALSE(ev.has_error);
}

TEST(OpenAIProtocol_ParseSse, FinishReason) {
    auto ev = parse_sse_line(
        R"(data: {"choices":[{"delta":{},"finish_reason":"stop"}]})");
    ASSERT_TRUE(ev.finish_reason.has_value());
    EXPECT_EQ(*ev.finish_reason, "stop");
}

TEST(OpenAIProtocol_ParseSse, DoneSentinel) {
    auto ev = parse_sse_line("data: [DONE]");
    EXPECT_TRUE(ev.is_done);
    EXPECT_FALSE(ev.delta_content.has_value());
}

TEST(OpenAIProtocol_ParseSse, EmptyLineNoEffect) {
    auto ev = parse_sse_line("");
    EXPECT_FALSE(ev.is_done);
    EXPECT_FALSE(ev.delta_content.has_value());
}

TEST(OpenAIProtocol_ParseSse, NonDataLineIgnored) {
    auto ev = parse_sse_line("event: ping");
    EXPECT_FALSE(ev.is_done);
    EXPECT_FALSE(ev.delta_content.has_value());
}

TEST(OpenAIProtocol_ParseSse, MalformedJsonNoCrash) {
    auto ev = parse_sse_line("data: {malformed");
    EXPECT_FALSE(ev.delta_content.has_value());
    EXPECT_FALSE(ev.is_done);
}

TEST(OpenAIProtocol_ParseSse, ErrorPayload) {
    auto ev = parse_sse_line(R"(data: {"error":{"message":"rate limit"}})");
    EXPECT_TRUE(ev.has_error);
    EXPECT_NE(ev.error_detail.find("rate limit"), std::string::npos);
}

// ===== map_http_status =====

TEST(OpenAIProtocol_HttpStatus, Ok2xx_NoError) {
    EXPECT_FALSE(map_http_status(200, "ok").has_value());
    EXPECT_FALSE(map_http_status(204, "").has_value());
}

TEST(OpenAIProtocol_HttpStatus, 401_AuthFailed) {
    auto e = map_http_status(401, "auth body");
    ASSERT_TRUE(e.has_value());
    EXPECT_EQ(e->code, static_cast<int>(E::kLlmAuthFailed));
}

TEST(OpenAIProtocol_HttpStatus, 429_RateLimited) {
    auto e = map_http_status(429, "limited");
    ASSERT_TRUE(e.has_value());
    EXPECT_EQ(e->code, static_cast<int>(E::kLlmRateLimited));
}

TEST(OpenAIProtocol_HttpStatus, 500_GenericHttpError) {
    auto e = map_http_status(500, "server err");
    ASSERT_TRUE(e.has_value());
    EXPECT_EQ(e->code, static_cast<int>(E::kHttpStatusError));
}

// ===== normalize_base_url =====

TEST(OpenAIProtocol_NormalizeUrl, StripsTrailingSlashes) {
    EXPECT_EQ(normalize_base_url("https://api.openai.com/v1"),
              "https://api.openai.com/v1");
    EXPECT_EQ(normalize_base_url("https://api.openai.com/v1/"),
              "https://api.openai.com/v1");
    EXPECT_EQ(normalize_base_url("https://api.openai.com/v1///"),
              "https://api.openai.com/v1");
}

TEST(OpenAIProtocol_NormalizeUrl, EmptyUnchanged) {
    EXPECT_EQ(normalize_base_url(""), "");
}

}  // namespace bagu::llm::openai_protocol
