#pragma once

#include <string>

namespace bagu::cli {

struct RemindCliOptions {
    std::string topic;          // 空 = 全部
    int threshold = 1;          // 到期数 < threshold 时不通知
    bool quiet = false;         // 不打印到 stdout/stderr，只发通知
    bool dry_run = false;       // 仅打印，不真发通知
};

int run_remind(const RemindCliOptions& opts);

}  // namespace bagu::cli
