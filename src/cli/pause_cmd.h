#pragma once

#include <cstdint>
#include <string>

namespace bagu::cli {

struct PauseCliOptions {
    int64_t card_id = 0;          // > 0 = 单卡
    std::string topic;             // 非空 = 整个 topic
    bool unpause = false;          // false = 暂停 / true = 恢复
};

int run_pause(const PauseCliOptions& opts);

}  // namespace bagu::cli
