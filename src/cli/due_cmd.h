#pragma once

#include <string>

namespace bagu::cli {

struct DueCliOptions {
    std::string topic;   // 空 = 全部主题
};

int run_due(const DueCliOptions& opts);

}  // namespace bagu::cli
