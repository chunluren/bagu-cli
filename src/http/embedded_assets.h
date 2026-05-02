#pragma once

#include <cstddef>
#include <string_view>

namespace bagu::http {

/// 嵌入到二进制中的单个静态资源
struct EmbeddedAsset {
    const char* path;          // 形如 "/index.html" / "/assets/foo.js"
    const unsigned char* data;
    size_t size;
    const char* mime;
};

/// 按路径查找嵌入资源；找不到返回 nullptr
const EmbeddedAsset* lookup_embedded_asset(std::string_view path);

/// 嵌入资源总数（0 表示未启用嵌入）
size_t embedded_asset_count();

}  // namespace bagu::http
