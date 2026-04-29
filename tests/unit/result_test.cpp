#include "bagu/result.h"

#include <gtest/gtest.h>

#include <string>

namespace bagu {

TEST(Result, OkValue_AccessibleViaValue) {
    Result<int> r(42);
    EXPECT_TRUE(r.is_ok());
    EXPECT_FALSE(r.is_err());
    EXPECT_EQ(r.value(), 42);
}

TEST(Result, ErrValue_AccessibleViaError) {
    Result<int> r(Error{E::kInvalidArgument, "bad input"});
    EXPECT_FALSE(r.is_ok());
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(E::kInvalidArgument));
    EXPECT_EQ(r.error().message, "bad input");
}

TEST(Result, ValueOr_OnError_ReturnsDefault) {
    Result<int> r(Error{E::kInternal, "x"});
    EXPECT_EQ(r.value_or(99), 99);
}

TEST(Result, ValueOr_OnOk_ReturnsValue) {
    Result<int> r(7);
    EXPECT_EQ(r.value_or(99), 7);
}

TEST(Result, BoolConversion) {
    Result<int> ok(1);
    Result<int> err(Error{E::kUnknown, ""});
    EXPECT_TRUE(static_cast<bool>(ok));
    EXPECT_FALSE(static_cast<bool>(err));
}

TEST(Result, MakeOk) {
    auto r = make_ok(std::string("hello"));
    ASSERT_TRUE(r.is_ok());
    EXPECT_EQ(r.value(), "hello");
}

TEST(Result, MakeErr) {
    auto r = make_err<int>(E::kFileNotFound, "missing");
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(E::kFileNotFound));
    EXPECT_EQ(r.error().message, "missing");
}

TEST(Result, VoidSpecialization_Ok) {
    Result<void> r = Result<void>::ok();
    EXPECT_TRUE(r.is_ok());
}

TEST(Result, VoidSpecialization_Err) {
    Result<void> r(Error{E::kDbOpenFailed, "fail"});
    EXPECT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, static_cast<int>(E::kDbOpenFailed));
}

TEST(Error, ToExitCodeMapping) {
    EXPECT_EQ(to_exit_code(0), 0);
    EXPECT_EQ(to_exit_code(static_cast<int>(E::kInvalidArgument)), 1);   // 1001 → 1
    EXPECT_EQ(to_exit_code(static_cast<int>(E::kArgRequired)), 2);       // 2000 → 2
    EXPECT_EQ(to_exit_code(static_cast<int>(E::kConfigNotFound)), 3);
    EXPECT_EQ(to_exit_code(static_cast<int>(E::kDbOpenFailed)), 4);
    EXPECT_EQ(to_exit_code(static_cast<int>(E::kFileNotFound)), 7);
    EXPECT_EQ(to_exit_code(static_cast<int>(E::kParseFailed)), 8);
    EXPECT_EQ(to_exit_code(static_cast<int>(E::kInternal)), 1);          // 9000 → 1
}

}  // namespace bagu
