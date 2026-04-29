#pragma once

#include <filesystem>
#include <string>

#include "bagu/result.h"

namespace bagu::util {

/// 计算字符串的 SHA256 哈希（十六进制小写，64 字符）
std::string sha256(std::string_view data);

/// 计算文件内容的 SHA256
Result<std::string> sha256_file(const std::filesystem::path& path);

}  // namespace bagu::util
