#pragma once

#include <filesystem>
#include <string>

namespace bagu::util {

/// 展开 ~ 为家目录
std::filesystem::path expand_home(std::string_view path);

/// 数据目录（默认 ~/.bagu）
std::filesystem::path default_data_dir();

/// 数据库文件路径
std::filesystem::path default_db_path();

/// 配置文件路径
std::filesystem::path default_config_path();

/// 日志目录
std::filesystem::path default_log_dir();

}  // namespace bagu::util
