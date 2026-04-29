#pragma once

#include <string>

namespace bagu::cli {

/// `bagu list [topic]`
/// - topic 为空：列出所有主题汇总
/// - topic 非空：列出该主题的章节树
int run_list(const std::string& topic);

}  // namespace bagu::cli
