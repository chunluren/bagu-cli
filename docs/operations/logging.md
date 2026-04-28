# 日志规范

> **状态**：Approved
> **更新**：2026-04-29

---

## 1. 日志库

**spdlog**（FetchContent 拉取）。

支持的特性：
- 多级别（trace/debug/info/warn/error/critical）
- 多 sink（stdout / file / rotating file）
- 异步 / 同步
- 格式化（基于 fmt）

---

## 2. 日志级别

| 级别       | 用途                                    | 示例                                |
|----------|---------------------------------------|-----------------------------------|
| trace    | 极详细的调试信息（只有 dev 看）              | "进入函数 X, 参数=..."              |
| debug    | 调试信息（dev mode 显示）                  | "card_id=42, ease_factor=2.6"     |
| **info** | **正常运行的关键节点**                      | "导入完成：87 cards"                |
| warn     | 异常但可恢复                              | "LLM 超时，重试中"                   |
| error    | 错误，部分功能失败                          | "DB 连接失败"                       |
| critical | 致命错误，程序无法继续                       | "数据库 schema 损坏"                 |

---

## 3. 默认级别

| 场景            | 默认级别        | 覆盖方式                       |
|---------------|-------------|-----------------------------|
| 生产（用户跑）     | warn         | --verbose 提到 debug          |
| 开发            | debug        | SPDLOG_LEVEL=trace          |
| CI 测试         | error        |                             |
| Sanitizer 构建  | debug        |                             |

```cpp
// 启动时设置
auto level = cfg.verbose ? spdlog::level::debug : spdlog::level::warn;
spdlog::set_level(level);

// 环境变量也可以
// SPDLOG_LEVEL=debug ./bagu
```

---

## 4. 日志输出

### 4.1 多 sink

```cpp
// 同时输出到 stderr 和文件
auto stderr_sink = std::make_shared<spdlog::sinks::stderr_color_sink_mt>();
stderr_sink->set_level(spdlog::level::warn);   // stderr 只显示 warn 以上

auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
    "~/.bagu/logs/bagu.log",
    10 * 1024 * 1024,    // 10MB / file
    5);                   // 保留 5 个
file_sink->set_level(spdlog::level::trace);     // 文件记全部

auto logger = std::make_shared<spdlog::logger>(
    "bagu", spdlog::sinks_init_list{stderr_sink, file_sink});
spdlog::set_default_logger(logger);
```

### 4.2 格式

```cpp
spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%n] %v");
// 输出例：
// [2026-04-29 12:34:56.789] [info] [bagu] 导入完成
```

---

## 5. 内容规范

### 5.1 用结构化 + 上下文

```cpp
// ✓ 好
spdlog::info("import: file={} cards={} elapsed_ms={}",
    file_path, count, ms);

// ✗ 差
spdlog::info("imported the file");
```

### 5.2 用语
- 中文为主（用户能看的）
- 关键词、ID、字段名用英文
- 错误信息要可操作（"重试" / "检查 X" / "联系作者"）

### 5.3 不要泄密

```cpp
// ✗ 严禁
spdlog::info("API key: {}", api_key);

// ✓
spdlog::info("API key loaded from env: {}", env_name);
```

### 5.4 性能

- **TRACE / DEBUG 级别在生产被过滤** — 调用 `spdlog::debug(...)` 即使被过滤也有少量开销
- 大对象格式化前先判断级别：
```cpp
if (spdlog::should_log(spdlog::level::debug)) {
    spdlog::debug("big object: {}", big_obj.dump());
}
```

---

## 6. 关键路径必须记日志

每个用户命令的入口和结束：

```cpp
int CommandImport::execute(...) {
    spdlog::info("开始导入 path={}", path);
    auto start = std::chrono::steady_clock::now();

    auto result = svc.import(path);

    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);

    if (result.is_ok()) {
        spdlog::info("导入完成 cards={} elapsed_ms={}",
            result.value().count, elapsed.count());
        return 0;
    } else {
        spdlog::error("导入失败 reason={}", result.error().message);
        return result.error().code;
    }
}
```

---

## 7. 常用模式

### 7.1 函数边界
```cpp
spdlog::trace(">> CardDao::find_by_id id={}", id);
// ...
spdlog::trace("<< CardDao::find_by_id id={} found={}", id, found);
```

### 7.2 异常 / 失败
```cpp
catch (const std::exception& e) {
    spdlog::error("解析失败 file={} error={}", path, e.what());
}

if (!result.ok()) {
    spdlog::warn("操作失败但已重试 attempt={} max={}", attempt, max);
}
```

### 7.3 重要状态变化
```cpp
spdlog::info("schema migration {} -> {}", current_version, target_version);
```

### 7.4 审计（重要操作）
```cpp
spdlog::info("audit: user_action=delete_card card_id={}", id);
```

---

## 8. 不要做

❌ **不要在循环里 INFO**
```cpp
for (auto& card : cards) {
    spdlog::info("processing card {}", card.id);   // ✗ 日志爆炸
}
```
改成：
```cpp
spdlog::info("processing {} cards", cards.size());
for (auto& card : cards) {
    spdlog::trace("card {}", card.id);
}
```

❌ **不要打印调试 print**
```cpp
std::cout << "here, x=" << x << "\n";   // ✗
spdlog::debug("here x={}", x);          // ✓
```

❌ **不要打印敏感信息**（API key、密码、个人信息）

❌ **不要忽略错误日志**
```cpp
catch (...) {
    // 默默吞掉，绝对禁止
}
```

---

## 9. 日志文件管理

### 9.1 路径
- `~/.bagu/logs/bagu.log`
- 通过 rotating sink 自动轮转
- 单文件 10 MB，保留 5 个 → 总 50 MB

### 9.2 清理
```bash
bagu log clean         # （未来命令）清理日志
```

---

## 10. TUI 模式特殊处理

TUI 模式（review/interview）下 stderr 输出会破坏界面：

```cpp
void enter_tui_mode() {
    // 1. 把 stderr sink 换成文件
    auto file_only = std::make_shared<spdlog::sinks::basic_file_sink_mt>(
        "~/.bagu/logs/tui.log");
    spdlog::set_default_logger(
        std::make_shared<spdlog::logger>("tui", file_only));
}
```

调试 TUI 时另开终端 `tail -f ~/.bagu/logs/tui.log`。

---

## 11. 测试中的日志

测试中默认关闭日志：
```cpp
class BaguTest : public ::testing::Test {
protected:
    void SetUp() override {
        spdlog::set_level(spdlog::level::off);
    }
};
```

调试时临时打开：
```cpp
TEST_F(MyTest, ...) {
    spdlog::set_level(spdlog::level::debug);
    // ...
}
```

---

## 12. JSON 日志（可选）

如果将来要做日志收集 / 分析：
```cpp
spdlog::set_pattern(R"({"ts":"%Y-%m-%dT%H:%M:%S.%e%z","level":"%l","msg":"%v"})");
```

---

## 13. 相关文档

- [spdlog 文档](https://github.com/gabime/spdlog)
- [错误处理](./error-handling.md)
