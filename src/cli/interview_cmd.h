#pragma once

#include <string>

namespace bagu::cli {

struct InterviewCliOptions {
    std::string topic;
    int num = 5;
    std::string provider;     // 覆盖配置
    std::string model;        // 覆盖配置
    std::string profile;      // [llm.profiles.<name>] 段
};

int run_interview(const InterviewCliOptions& opts);

}  // namespace bagu::cli
