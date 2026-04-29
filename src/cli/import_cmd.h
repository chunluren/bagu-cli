#pragma once

#include <string>

namespace bagu::cli {

/// 执行 `bagu import <path> [--force]`
int run_import(const std::string& path, bool force);

}  // namespace bagu::cli
