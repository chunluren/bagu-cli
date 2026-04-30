# ADR-0005: HTTP server 选用 cpp-httplib

- **状态**: Proposed
- **日期**: 2026-04-30
- **作者**: Li Yan

## 背景

按 [ADR-0004](./0004-add-http-server-mode.md) 决定加 HTTP server，需要选具体 C++ HTTP 库。

需求：
- 单头文件 / 易集成（用 FetchContent）
- 同步阻塞 API 即可（CLI 工具，单线程够用）
- 支持 SSE
- 不引入额外依赖（已有 OpenSSL，但其它库越少越好）
- Linux + macOS 即可（不必 Windows）

## 决策

**选用 [yhirose/cpp-httplib](https://github.com/yhirose/cpp-httplib)。**

## 备选方案

| 方案 | 优点 | 缺点 | 不选理由 |
|----|----|----|----|
| **cpp-httplib** | 单头文件、API 极简、SSE 支持、活跃维护 | 性能不如 nginx 级 | — |
| Boost.Beast | 功能全、async 好 | Boost 依赖巨大 | 项目刚开始用 boost 不划算 |
| crow | Express 风格、简单 | 模板编译慢、依赖 boost::asio | 同上 |
| Drogon | 性能好 | 依赖多、学习曲线陡 | 个人工具不需要 |
| Pistache | 现代、async | API 较复杂 | 不如 httplib 简单 |
| 自己写 | 完全可控 | 重新发明轮子 | 不值 |

## 后果

### 正面

- **单头文件 `httplib.h`** — FetchContent 拉取后无任何额外编译目标
- **API 极简**：
  ```cpp
  httplib::Server svr;
  svr.Get("/api/topics", [](const Request& req, Response& res) {
      res.set_content(json, "application/json");
  });
  svr.listen("127.0.0.1", 8780);
  ```
- **SSE 内置支持** — `set_chunked_content_provider`
- **HTTPS 可选**（需要 OpenSSL，已有）
- **小**：仅 ~30KB 二进制开销

### 负面

- **同步阻塞 IO**：每个请求一线程（默认线程池），不适合 10000+ QPS
  - 个人工具无影响
- **不支持 HTTP/2**
  - 不需要

### 中性

- 不维护 OpenAPI codegen（手写文档）
- 性能足够个人工具（实测单线程 10K QPS+）

## 实施

```cmake
FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.5
    GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(cpp-httplib)

target_link_libraries(bagu_lib PUBLIC httplib::httplib)
```

代码示例（生产用法）：
```cpp
#include <httplib.h>

namespace bagu::http {
class HttpServer {
public:
    HttpServer(db::Database& db, std::string bind, int port);
    void register_routes();
    int run();
private:
    httplib::Server svr_;
    db::Database& db_;
};
}
```

## 相关链接

- [cpp-httplib README](https://github.com/yhirose/cpp-httplib)
- [ADR-0004](./0004-add-http-server-mode.md)
- [http-api-spec.md](../http-api-spec.md)
