#include "llm/config_loader.h"

#include <gtest/gtest.h>

#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace bagu::llm {

namespace fs = std::filesystem;

class ConfigLoaderTest : public ::testing::Test {
protected:
    void SetUp() override {
        tmp_ = fs::temp_directory_path() /
            ("bagu-cfg-test-" + std::to_string(::getpid()));
        fs::create_directories(tmp_);
        ::setenv("BAGU_HOME", tmp_.string().c_str(), 1);
    }

    void TearDown() override {
        ::unsetenv("BAGU_HOME");
        ::unsetenv("OPENAI_API_KEY");
        ::unsetenv("OLLAMA_KEY");
        ::unsetenv("CHEAP_KEY");
        std::error_code ec;
        fs::remove_all(tmp_, ec);
    }

    void write_config(const std::string& contents) {
        std::ofstream(tmp_ / "config.toml") << contents;
    }

    fs::path tmp_;
};

// ===== 默认行为 =====

TEST_F(ConfigLoaderTest, NoConfigFile_FallsBackToOpenAI) {
    auto r = load_config({});
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().provider, "openai");
}

TEST_F(ConfigLoaderTest, DefaultSection_LoadsAllFields) {
    write_config(R"(
[llm]
provider = "openai"
model = "gpt-4o-mini"
base_url = "https://api.openai.com/v1"
)");
    auto r = load_config({});
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().provider, "openai");
    EXPECT_EQ(r.value().model, "gpt-4o-mini");
    EXPECT_EQ(r.value().base_url, "https://api.openai.com/v1");
}

// ===== Override（CLI 最高优先） =====

TEST_F(ConfigLoaderTest, Override_BeatsDefault) {
    write_config(R"(
[llm]
provider = "openai"
model = "gpt-4o-mini"
)");
    ConfigOverride ovr; ovr.provider = "ollama"; ovr.model = "qwen2.5";
    auto r = load_config(ovr);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().provider, "ollama");
    EXPECT_EQ(r.value().model, "qwen2.5");
}

// ===== Profile 段 =====

TEST_F(ConfigLoaderTest, Profile_OverridesDefault) {
    write_config(R"(
[llm]
provider = "openai"
model = "gpt-4o-mini"
base_url = "https://api.openai.com/v1"

[llm.profiles.cheap]
provider = "ollama"
model = "llama3.2"
base_url = "http://localhost:11434/v1"
)");
    ConfigOverride ovr; ovr.profile = "cheap";
    auto r = load_config(ovr);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().provider, "ollama");
    EXPECT_EQ(r.value().model, "llama3.2");
    EXPECT_EQ(r.value().base_url, "http://localhost:11434/v1");
}

TEST_F(ConfigLoaderTest, Profile_PartialOverride_InheritsFromDefault) {
    write_config(R"(
[llm]
provider = "openai"
model = "gpt-4o-mini"
base_url = "https://api.openai.com/v1"

[llm.profiles.serious]
model = "gpt-4o"
)");
    ConfigOverride ovr; ovr.profile = "serious";
    auto r = load_config(ovr);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().provider, "openai");                          // 继承
    EXPECT_EQ(r.value().model, "gpt-4o");                              // 覆盖
    EXPECT_EQ(r.value().base_url, "https://api.openai.com/v1");       // 继承
}

TEST_F(ConfigLoaderTest, OverrideBeatsProfile) {
    write_config(R"(
[llm]
provider = "openai"

[llm.profiles.cheap]
provider = "ollama"
model = "llama3.2"
)");
    ConfigOverride ovr; ovr.profile = "cheap"; ovr.model = "qwen2.5";
    auto r = load_config(ovr);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().provider, "ollama");      // from profile
    EXPECT_EQ(r.value().model, "qwen2.5");        // from override (beats profile's llama3.2)
}

TEST_F(ConfigLoaderTest, UnknownProfile_ReturnsError) {
    write_config(R"(
[llm]
provider = "openai"
)");
    ConfigOverride ovr; ovr.profile = "doesnotexist";
    auto r = load_config(ovr);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(E::kConfigInvalid));
    EXPECT_NE(r.error().message.find("doesnotexist"), std::string::npos);
}

// ===== api_key_env =====

TEST_F(ConfigLoaderTest, ApiKeyEnv_LoadedFromEnv) {
    ::setenv("OPENAI_API_KEY", "sk-test123", 1);
    write_config(R"(
[llm]
provider = "openai"
api_key_env = "OPENAI_API_KEY"
)");
    auto r = load_config({});
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().api_key, "sk-test123");
}

TEST_F(ConfigLoaderTest, MissingApiKeyEnv_ReturnsError) {
    ::unsetenv("MISSING_KEY");
    write_config(R"(
[llm]
provider = "openai"
api_key_env = "MISSING_KEY"
)");
    auto r = load_config({});
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(E::kLlmAuthFailed));
}

TEST_F(ConfigLoaderTest, OllamaProfile_NoApiKeyEnvOk) {
    write_config(R"(
[llm]
provider = "openai"
api_key_env = "OPENAI_API_KEY"

[llm.profiles.local]
provider = "ollama"
model = "llama3.2"
)");
    ::setenv("OPENAI_API_KEY", "x", 1);  // 让 default 段 OK
    ConfigOverride ovr; ovr.profile = "local";
    auto r = load_config(ovr);
    // ollama 不需要 key；但 default 段还指向 OPENAI_API_KEY，
    // resolve_api_key_env 优先 profile（profile 没有 api_key_env）→ 回退 default
    // default 的 api_key_env=OPENAI_API_KEY，但 provider 已经被 profile 改成 ollama
    // 所以 missing-env 检查会跳过 → OK
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().provider, "ollama");
}

TEST_F(ConfigLoaderTest, ProfileWithOwnApiKeyEnv) {
    ::setenv("CHEAP_KEY", "cheap-secret", 1);
    write_config(R"(
[llm]
provider = "openai"
api_key_env = "OPENAI_API_KEY"

[llm.profiles.cheap]
provider = "openai-compat"
base_url = "https://cheap.example.com/v1"
api_key_env = "CHEAP_KEY"
)");
    ConfigOverride ovr; ovr.profile = "cheap";
    auto r = load_config(ovr);
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value().api_key, "cheap-secret");
}

// ===== list_profiles =====

TEST_F(ConfigLoaderTest, ListProfiles_NoConfig_OnlyDefault) {
    auto r = list_profiles();
    ASSERT_TRUE(r.is_ok());
    ASSERT_EQ(r.value().size(), 1u);
    EXPECT_EQ(r.value()[0], "default");
}

TEST_F(ConfigLoaderTest, ListProfiles_WithProfiles) {
    write_config(R"(
[llm]
provider = "openai"

[llm.profiles.cheap]
provider = "ollama"

[llm.profiles.serious]
model = "gpt-4o"
)");
    auto r = list_profiles();
    ASSERT_TRUE(r.is_ok());
    auto& v = r.value();
    EXPECT_EQ(v.size(), 3u);   // default + cheap + serious
    EXPECT_EQ(v[0], "default");
    // 其他俩顺序由 toml 库决定，只验证集合
    std::set<std::string> rest(v.begin() + 1, v.end());
    EXPECT_EQ(rest, (std::set<std::string>{"cheap", "serious"}));
}

}  // namespace bagu::llm
