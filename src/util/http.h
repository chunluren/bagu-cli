#pragma once

#include <functional>
#include <string>
#include <vector>

#include "bagu/result.h"

namespace bagu::util {

struct HttpResponse {
    int status_code = 0;
    std::string body;
};

/// 同步 HTTP POST，返回完整响应
Result<HttpResponse> http_post_json(
    const std::string& url,
    const std::string& json_body,
    const std::vector<std::string>& headers,
    int timeout_seconds = 60);

/// 流式 HTTP POST（用于 Server-Sent Events）
/// on_data 收到的是按行拆分的原始数据（含 "data: ..." 等 SSE 头）
Result<int> http_post_json_stream(
    const std::string& url,
    const std::string& json_body,
    const std::vector<std::string>& headers,
    std::function<void(const std::string& line)> on_line,
    int timeout_seconds = 120);

}  // namespace bagu::util
