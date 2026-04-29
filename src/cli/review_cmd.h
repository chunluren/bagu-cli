#pragma once

#include <string>

namespace bagu::cli {

struct ReviewOptions {
    std::string topic;
    int num = 0;          // 0 = 默认值（按 daily_target 等）
    bool new_only = false;
    bool all = false;     // --all：取所有到期，不按 num 限制
};

int run_review(const ReviewOptions& opts);

}  // namespace bagu::cli
