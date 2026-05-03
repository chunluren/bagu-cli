#pragma once

#include <string>

namespace bagu::cli {

struct ExportCliOptions {
    std::string format = "anki";   // anki（目前唯一）
    std::string topic;             // 空 = 全部主题
    std::string output;            // 空 = stdout
    bool include_section_cards = false;
};

int run_export(const ExportCliOptions& opts);

}  // namespace bagu::cli
