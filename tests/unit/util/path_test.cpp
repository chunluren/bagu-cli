#include "util/path.h"

#include <gtest/gtest.h>

#include <cstdlib>

namespace bagu::util {

class PathTest : public ::testing::Test {
protected:
    void SetUp() override {
        old_home_ = std::getenv("HOME");
        if (old_home_) old_home_str_ = old_home_;

        old_bagu_home_ = std::getenv("BAGU_HOME");
        if (old_bagu_home_) old_bagu_home_str_ = old_bagu_home_;

        ::setenv("HOME", "/home/testuser", 1);
        ::unsetenv("BAGU_HOME");
    }

    void TearDown() override {
        if (old_home_) ::setenv("HOME", old_home_str_.c_str(), 1);
        else ::unsetenv("HOME");

        if (old_bagu_home_) ::setenv("BAGU_HOME", old_bagu_home_str_.c_str(), 1);
        else ::unsetenv("BAGU_HOME");
    }

    const char* old_home_ = nullptr;
    std::string old_home_str_;
    const char* old_bagu_home_ = nullptr;
    std::string old_bagu_home_str_;
};

TEST_F(PathTest, ExpandHome_TildeAlone_ReturnsHome) {
    EXPECT_EQ(expand_home("~").string(), "/home/testuser");
}

TEST_F(PathTest, ExpandHome_TildeSlash_ExpandsCorrectly) {
    EXPECT_EQ(expand_home("~/foo/bar").string(), "/home/testuser/foo/bar");
}

TEST_F(PathTest, ExpandHome_NoTilde_ReturnsAsIs) {
    EXPECT_EQ(expand_home("/etc/passwd").string(), "/etc/passwd");
    EXPECT_EQ(expand_home("relative/path").string(), "relative/path");
}

TEST_F(PathTest, ExpandHome_TildeWithoutSlash_ReturnsAsIs) {
    // ~user 这种 GNU 形式不支持，按字面返回
    EXPECT_EQ(expand_home("~user").string(), "~user");
}

TEST_F(PathTest, ExpandHome_Empty_ReturnsEmpty) {
    EXPECT_EQ(expand_home("").string(), "");
}

TEST_F(PathTest, DefaultDataDir_NoBaguHome_UsesHomeDotBagu) {
    EXPECT_EQ(default_data_dir().string(), "/home/testuser/.bagu");
}

TEST_F(PathTest, DefaultDataDir_BaguHomeSet_UsesIt) {
    ::setenv("BAGU_HOME", "/tmp/custom-bagu", 1);
    EXPECT_EQ(default_data_dir().string(), "/tmp/custom-bagu");
}

TEST_F(PathTest, DefaultDataDir_BaguHomeWithTilde_Expands) {
    ::setenv("BAGU_HOME", "~/my-bagu", 1);
    EXPECT_EQ(default_data_dir().string(), "/home/testuser/my-bagu");
}

TEST_F(PathTest, DefaultDbPath_IsBaguDb) {
    EXPECT_EQ(default_db_path().string(), "/home/testuser/.bagu/bagu.db");
}

TEST_F(PathTest, DefaultConfigPath_IsConfigToml) {
    EXPECT_EQ(default_config_path().string(), "/home/testuser/.bagu/config.toml");
}

TEST_F(PathTest, DefaultLogDir_IsBaguLogs) {
    EXPECT_EQ(default_log_dir().string(), "/home/testuser/.bagu/logs");
}

}  // namespace bagu::util
