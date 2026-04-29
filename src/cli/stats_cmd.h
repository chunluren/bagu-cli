#pragma once

#include <string>

namespace bagu::cli {

struct StatsOptions {
    std::string topic;
    bool heatmap = false;
    int days = 90;
};

int run_stats(const StatsOptions& opts);

}  // namespace bagu::cli
