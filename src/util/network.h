#pragma once

#include <string>
#include <vector>

namespace bagu::util {

/// 列出本机的 IPv4 地址（不含 127.x.x.x、不含 docker/虚拟桥）。
/// 用于 `bagu serve --bind 0.0.0.0` 启动时打印「手机能用的 URL」。
/// 失败 / 平台不支持 → 返回空 vector，不抛错。
std::vector<std::string> local_ipv4_addresses();

}  // namespace bagu::util
