# 错误处理与错误码

> **状态**：Approved
> **更新**：2026-04-29

---

## 1. 设计原则

1. **不抛异常** — 项目级约定，全程用 `Result<T>`
2. **错误显式** — 调用者必须处理或显式忽略
3. **不丢错** — 错误必须传播或日志记录
4. **可恢复 vs 不可恢复** — 区分清楚
5. **用户友好** — 错误信息让用户知道下一步该做什么

---

## 2. Result<T> 类型

### 2.1 定义

```cpp
namespace bagu {

struct Error {
    int code;                    // 错误码（见下表）
    std::string message;         // 简短描述（用户可见）
    std::string detail;          // 详细信息（日志用）

    // 链式错误（保留底层错因）
    std::shared_ptr<Error> cause;
};

template <typename T>
class [[nodiscard]] Result {
public:
    static Result ok(T value);
    static Result err(Error error);

    bool is_ok() const noexcept;
    bool is_err() const noexcept;
    explicit operator bool() const noexcept { return is_ok(); }

    T& value();                       // 不安全，未检查直接 abort
    const T& value() const;
    T value_or(T default_value) const;

    const Error& error() const;

    // monadic operations
    template <typename F>
    auto map(F&& f) -> Result<decltype(f(std::declval<T>()))>;

    template <typename F>
    auto and_then(F&& f);             // F 返回 Result<U>

    template <typename F>
    Result or_else(F&& f);

private:
    std::variant<T, Error> data_;
};

// void 特化
template <>
class Result<void> { ... };

}  // namespace bagu
```

### 2.2 用法

```cpp
Result<Card> CardDao::find_by_id(int64_t id) {
    auto stmt = db_->prepare(...);
    if (!stmt) {
        return Result<Card>::err({E::kDbPrepareFailed, "prepare failed"});
    }

    if (!stmt.step()) {
        return Result<Card>::err({E::kCardNotFound,
            std::format("card {} not found", id)});
    }

    return Result<Card>::ok(parse_card(stmt));
}

// 调用方式 1：传统 if
auto r = card_dao.find_by_id(42);
if (r.is_err()) {
    spdlog::error("查找失败: {}", r.error().message);
    return r.error().code;
}
const Card& card = r.value();

// 调用方式 2：链式
auto title = card_dao.find_by_id(42)
    .map([](const Card& c) { return c.question; })
    .value_or("(not found)");
```

### 2.3 [[nodiscard]] 强制处理

```cpp
result_dao.insert(card);   // 编译警告：discarding return value
auto r = result_dao.insert(card);
(void)r;                   // 显式忽略也至少要这样
```

---

## 3. 错误码体系

### 3.1 错误码分段

```
0       OK
1xxx    通用错误
2xxx    参数 / 输入错误
3xxx    配置错误
4xxx    数据库错误
5xxx    网络 / LLM API 错误
6xxx    用户取消 / 中断
7xxx    文件系统错误
8xxx    解析错误
9xxx    内部错误（bug）
```

### 3.2 完整错误码表

`include/bagu/error_codes.h`：
```cpp
namespace bagu {

enum class E : int {
    // ===== 通用 =====
    kOk = 0,
    kUnknown = 1000,
    kInvalidArgument = 1001,
    kNotImplemented = 1002,

    // ===== 参数 / 输入 =====
    kArgRequired = 2000,
    kArgInvalidValue = 2001,
    kCommandUnknown = 2002,

    // ===== 配置 =====
    kConfigNotFound = 3000,
    kConfigInvalid = 3001,
    kConfigMissingField = 3002,
    kConfigPermissionDenied = 3003,

    // ===== 数据库 =====
    kDbNotFound = 4000,
    kDbOpenFailed = 4001,
    kDbSchemaError = 4002,
    kDbMigrationFailed = 4003,
    kDbPrepareFailed = 4010,
    kDbExecuteFailed = 4011,
    kDbConstraintViolation = 4012,
    kDbTransactionFailed = 4013,
    kCardNotFound = 4020,
    kTopicNotFound = 4021,
    kChapterNotFound = 4022,
    kReviewNotFound = 4023,

    // ===== 网络 / LLM =====
    kNetworkError = 5000,
    kHttpTimeout = 5001,
    kHttpStatusError = 5002,
    kLlmAuthFailed = 5010,
    kLlmRateLimited = 5011,
    kLlmInvalidResponse = 5012,
    kLlmProviderUnknown = 5013,

    // ===== 用户取消 =====
    kUserCancelled = 6000,
    kUserTimeout = 6001,

    // ===== 文件系统 =====
    kFileNotFound = 7000,
    kFileReadFailed = 7001,
    kFileWriteFailed = 7002,
    kFilePermissionDenied = 7003,
    kDirNotFound = 7010,
    kDirCreateFailed = 7011,

    // ===== 解析 =====
    kParseFailed = 8000,
    kParseInvalidFormat = 8001,
    kParseEmptyDocument = 8002,

    // ===== 内部 =====
    kInternal = 9000,
    kAssertionFailed = 9001,
    kBug = 9999,
};

const char* error_code_name(E code);

}  // namespace bagu
```

### 3.3 错误码 → CLI 退出码

```cpp
int to_exit_code(int err_code) {
    if (err_code == 0) return 0;
    if (err_code >= 9000) return 1;
    if (err_code >= 8000) return 8;
    if (err_code >= 7000) return 7;
    if (err_code >= 6000) return 6;
    if (err_code >= 5000) return 5;
    if (err_code >= 4000) return 4;
    if (err_code >= 3000) return 3;
    if (err_code >= 2000) return 2;
    return 1;
}
```

---

## 4. 错误信息规范

### 4.1 三层信息

| 层级    | 内容          | 示例                           |
|------|-------------|------------------------------|
| code | 数字错误码      | E::kDbNotFound (4000)         |
| message | 用户可见的简短信息 | "数据库不存在"                  |
| detail  | 详细信息（日志用） | "path=/home/.../bagu.db, errno=2" |

### 4.2 显示格式

```
Error: <message> (E<code>)

  <detail>

提示：<actionable suggestion>
```

实例：
```
Error: 数据库不存在 (E4000)

  path: /home/user/.bagu/bagu.db

提示：请先运行 `bagu init` 初始化数据库
```

### 4.3 写错误信息要可操作

| ❌ 没用              | ✓ 可操作                              |
|----------------|--------------------------------|
| 出错了             | 数据库未初始化，请运行 `bagu init`         |
| Connection failed | 无法连接 OpenAI，请检查 OPENAI_API_KEY     |
| Invalid input    | 主题名只能包含小写字母和短横线（如 mysql）       |

---

## 5. 错误传播

### 5.1 简单转发
```cpp
Result<int> upper_func() {
    auto r = lower_func();
    if (r.is_err()) return Result<int>::err(r.error());
    return Result<int>::ok(r.value() * 2);
}
```

### 5.2 错误链（context）

底层错误向上传时，加上上下文不丢原因：
```cpp
Result<Topic> ImportService::import(const std::string& path) {
    auto content = file::read(path);
    if (content.is_err()) {
        return Result<Topic>::err({
            .code = (int)E::kFileReadFailed,
            .message = "无法读取文件",
            .detail = "path=" + path,
            .cause = std::make_shared<Error>(content.error())
        });
    }
    // ...
}

// 显示时
void print_error_chain(const Error& e, int depth = 0) {
    std::string indent(depth * 2, ' ');
    fmt::print(stderr, "{}Error: {} (E{})\n", indent, e.message, e.code);
    if (!e.detail.empty()) {
        fmt::print(stderr, "{}  {}\n", indent, e.detail);
    }
    if (e.cause) {
        fmt::print(stderr, "{}Caused by:\n", indent);
        print_error_chain(*e.cause, depth + 1);
    }
}
```

### 5.3 宏简化（可选）

```cpp
#define BAGU_TRY(expr) \
    ({ \
        auto _r = (expr); \
        if (_r.is_err()) return Result<...>::err(_r.error()); \
        std::move(_r.value()); \
    })

// 用法（GCC/Clang 扩展）：
auto x = BAGU_TRY(some_func());
```

或不用宏：
```cpp
if (auto r = some_func(); r.is_err()) return forward_error(r);
```

---

## 6. 何时该 abort（不可恢复）

只有以下情况 abort：
- 内存分配失败
- 严重的逻辑错（违反不变量）
- assertion 失败

```cpp
void do_work() {
    BAGU_ASSERT(initialized_, "must call init first");
    // ...
}
```

`BAGU_ASSERT`：
```cpp
#define BAGU_ASSERT(cond, msg) \
    do { \
        if (!(cond)) { \
            spdlog::critical("assertion failed: {} ({}:{})", msg, __FILE__, __LINE__); \
            std::abort(); \
        } \
    } while(0)
```

---

## 7. 用户中断（Ctrl+C）

```cpp
#include <csignal>
#include <atomic>

std::atomic<bool> g_interrupted{false};

void install_signal_handler() {
    std::signal(SIGINT, [](int) {
        g_interrupted.store(true);
    });
}

// 长循环里检查
while (process_more) {
    if (g_interrupted.load()) {
        return Result<...>::err({E::kUserCancelled, "操作已取消"});
    }
    // ...
}
```

---

## 8. CLI 层错误展示

```cpp
int main(int argc, char** argv) {
    install_signal_handler();

    try {
        // 不该走到这里，但兜底
        auto result = run(argc, argv);
        if (result.is_err()) {
            print_error_chain(result.error());
            return to_exit_code(result.error().code);
        }
        return 0;
    } catch (const std::exception& e) {
        spdlog::critical("uncaught exception: {}", e.what());
        return 1;
    } catch (...) {
        spdlog::critical("uncaught unknown exception");
        return 1;
    }
}
```

---

## 9. 日志中记录错误

```cpp
if (r.is_err()) {
    spdlog::error("操作失败 op={} code={} msg={} detail={}",
        "import", r.error().code, r.error().message, r.error().detail);
}
```

---

## 10. 测试错误路径

每个错误码至少有一个测试覆盖：

```cpp
TEST(CardDao, FindById_NotFound_ReturnsCardNotFoundError) {
    auto r = dao.find_by_id(99999);
    ASSERT_TRUE(r.is_err());
    EXPECT_EQ(r.error().code, (int)E::kCardNotFound);
}
```

---

## 11. 相关文档

- [日志规范](./logging.md)
- [CLI 规范](../architecture/cli-spec.md)
- [编码规范](../development/coding-standards.md)
