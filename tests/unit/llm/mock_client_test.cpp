#include "llm/mock_client.h"

#include <gtest/gtest.h>

namespace bagu::llm {

TEST(MockClient, NoPreset_ReturnsError) {
    MockClient c;
    ChatRequest req;
    req.messages.push_back({"user", "hello"});
    auto r = c.chat(req);
    EXPECT_TRUE(r.is_err());
}

TEST(MockClient, PresetThenChat_ReturnsContent) {
    MockClient c;
    c.push_response("hello world");
    ChatRequest req;
    req.messages.push_back({"user", "hi"});
    auto r = c.chat(req);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().content, "hello world");
    EXPECT_EQ(r.value().finish_reason, "stop");
}

TEST(MockClient, RequestRecorded) {
    MockClient c;
    c.push_response("a");
    c.push_response("b");
    ChatRequest req;
    req.messages.push_back({"user", "q1"});
    c.chat(req);
    req.messages.push_back({"user", "q2"});
    c.chat(req);
    EXPECT_EQ(c.requests().size(), 2u);
    EXPECT_EQ(c.requests()[1].messages.size(), 2u);
}

TEST(MockClient, StreamCallsOnChunk) {
    MockClient c;
    c.push_response("piece");
    ChatRequest req;
    req.messages.push_back({"user", "x"});

    std::string accum;
    auto r = c.chat_stream(req, [&](const std::string& s) { accum += s; });
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(accum, "piece");
}

TEST(MockClient, ProviderName) {
    MockClient c;
    EXPECT_EQ(c.provider_name(), "mock");
}

TEST(Factory, UnknownProvider_ReturnsError) {
    ClientConfig cfg;
    cfg.provider = "xxxx";
    auto r = make_client(cfg);
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(E::kLlmProviderUnknown));
}

TEST(Factory, OpenAIProvider_BuildsClient) {
    ClientConfig cfg;
    cfg.provider = "openai";
    cfg.api_key = "sk-test";
    auto r = make_client(cfg);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value()->provider_name(), "openai");
}

TEST(Factory, OllamaProvider_BuildsClient) {
    ClientConfig cfg;
    cfg.provider = "ollama";
    auto r = make_client(cfg);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value()->provider_name(), "openai");  // 内部走 OpenAI 兼容
}

TEST(Factory, EmptyProvider_DefaultsToOpenAI) {
    ClientConfig cfg;
    cfg.api_key = "sk-test";
    auto r = make_client(cfg);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value()->provider_name(), "openai");
}

}  // namespace bagu::llm
