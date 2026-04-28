#pragma once

#include <memory>
#include <string>

namespace bagu {

/// 错误码（详见 docs/operations/error-handling.md）
enum class E : int {
    kOk = 0,

    // 通用
    kUnknown = 1000,
    kInvalidArgument = 1001,
    kNotImplemented = 1002,

    // 参数
    kArgRequired = 2000,
    kArgInvalidValue = 2001,

    // 配置
    kConfigNotFound = 3000,
    kConfigInvalid = 3001,
    kConfigMissingField = 3002,

    // 数据库
    kDbNotFound = 4000,
    kDbOpenFailed = 4001,
    kDbSchemaError = 4002,
    kDbMigrationFailed = 4003,
    kDbPrepareFailed = 4010,
    kDbExecuteFailed = 4011,

    // 文件系统
    kFileNotFound = 7000,
    kFileReadFailed = 7001,
    kFileWriteFailed = 7002,
    kDirNotFound = 7010,
    kDirCreateFailed = 7011,

    // 解析
    kParseFailed = 8000,

    // 内部
    kInternal = 9000,
};

/// 把错误码映射为 CLI 退出码
inline int to_exit_code(int err_code) {
    if (err_code == 0) return 0;
    if (err_code >= 9000) return 1;
    if (err_code >= 8000) return 8;
    if (err_code >= 7000) return 7;
    if (err_code >= 5000) return 5;
    if (err_code >= 4000) return 4;
    if (err_code >= 3000) return 3;
    if (err_code >= 2000) return 2;
    return 1;
}

/// 错误对象
struct Error {
    int code = static_cast<int>(E::kUnknown);
    std::string message;
    std::string detail;
    std::shared_ptr<Error> cause;

    Error() = default;
    Error(E c, std::string msg, std::string det = {})
        : code(static_cast<int>(c)),
          message(std::move(msg)),
          detail(std::move(det)) {}
};

}  // namespace bagu
