#pragma once

#include <cassert>
#include <utility>
#include <variant>

#include "bagu/error.h"

namespace bagu {

/// Result<T> 替代异常做错误传播
///
/// @code
/// Result<int> parse(const std::string& s) {
///     if (s.empty()) return Error{E::kInvalidArgument, "empty"};
///     return std::stoi(s);
/// }
///
/// auto r = parse("42");
/// if (r.is_err()) { ... }
/// int v = r.value();
/// @endcode
template <typename T>
class [[nodiscard]] Result {
public:
    Result(T value) : data_(std::move(value)) {}
    Result(Error err) : data_(std::move(err)) {}

    bool is_ok() const noexcept { return std::holds_alternative<T>(data_); }
    bool is_err() const noexcept { return std::holds_alternative<Error>(data_); }
    explicit operator bool() const noexcept { return is_ok(); }

    T& value() {
        assert(is_ok() && "Result::value() on error");
        return std::get<T>(data_);
    }
    const T& value() const {
        assert(is_ok() && "Result::value() on error");
        return std::get<T>(data_);
    }

    T value_or(T default_value) const {
        return is_ok() ? std::get<T>(data_) : std::move(default_value);
    }

    const Error& error() const {
        assert(is_err() && "Result::error() on ok");
        return std::get<Error>(data_);
    }

private:
    std::variant<T, Error> data_;
};

/// void 特化
template <>
class [[nodiscard]] Result<void> {
public:
    /// 默认构造表示成功（code=0）
    Result() { err_.code = 0; }
    Result(Error err) : err_(std::move(err)) {}

    bool is_ok() const noexcept { return err_.code == 0; }
    bool is_err() const noexcept { return err_.code != 0; }
    explicit operator bool() const noexcept { return is_ok(); }

    const Error& error() const {
        assert(is_err());
        return err_;
    }

    static Result<void> ok() { return Result<void>(); }

private:
    Error err_;
};

/// 简便构造
template <typename T>
inline Result<T> make_ok(T value) {
    return Result<T>(std::move(value));
}

template <typename T>
inline Result<T> make_err(E code, std::string msg, std::string detail = {}) {
    return Result<T>(Error{code, std::move(msg), std::move(detail)});
}

inline Result<void> make_err(E code, std::string msg, std::string detail = {}) {
    return Result<void>(Error{code, std::move(msg), std::move(detail)});
}

}  // namespace bagu
