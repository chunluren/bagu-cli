# 安全清单

> **状态**：Approved
> **更新**：2026-04-29

---

## 1. 威胁模型（Threat Model）

### 1.1 边界

```
┌────────────────────────────────────┐
│       用户机器（信任边界内）          │
│                                     │
│   bagu-cli ←→ ~/.bagu/bagu.db       │
│             ←→ markdown 文件         │
│             ←→ ~/.bagu/config.toml   │
└────────────────────────────────────┘
            ↓ HTTPS（信任边界外）
┌────────────────────────────────────┐
│   LLM API（OpenAI / Claude）         │
└────────────────────────────────────┘
```

### 1.2 资产

| 资产           | 敏感度        |
|--------------|-------------|
| API key       | 高           |
| 用户卡片数据    | 中（个人学习数据）|
| 配置文件        | 低           |

### 1.3 威胁清单

| 威胁                          | 严重性 | 概率 | 缓解                        |
|----------------------------|------|------|---------------------------|
| API key 泄漏                  | 高    | 中    | 用环境变量、不写日志             |
| 命令注入（用户输入）              | 中    | 低    | 不调 shell、参数严格校验          |
| SQL 注入                      | 高    | 低    | 用 prepared statement      |
| 路径遍历（import 任意文件）      | 中    | 中    | 校验路径在允许范围                |
| 反序列化攻击（恶意 markdown）   | 低    | 低    | 解析器隔离                        |
| 拒绝服务（巨大 markdown）        | 低    | 低    | 文件大小上限                      |
| LLM 提示注入                    | 低    | 中    | 使用方自负责（不影响他人）          |
| 数据库文件被读                  | 低    | 高    | 设计上不存敏感信息                  |

---

## 2. 安全要求清单

### 2.1 ✅ 必须做

#### 2.1.1 API Key 处理
- ✅ **只从环境变量读**，不存配置文件
  ```toml
  [llm]
  api_key_env = "OPENAI_API_KEY"   # ✓
  # api_key = "sk-xxx"              # ✗ 严禁
  ```
- ✅ 日志中**永不打印** API key
- ✅ 错误信息中**永不暴露** API key
- ✅ HTTP 请求 header 中正确传递

#### 2.1.2 SQL 安全
- ✅ **永远用 prepared statement + bind**
  ```cpp
  // ✓
  auto stmt = db_->prepare("SELECT * FROM card WHERE id = ?");
  stmt.bind(1, id);

  // ✗ 严禁
  std::string sql = "SELECT * FROM card WHERE id = " + std::to_string(id);
  ```

#### 2.1.3 文件路径校验
- ✅ `bagu import <path>` 校验路径
  - 必须存在
  - 必须是可读的常规文件 / 目录
  - 不允许 `..` 跨越（如果将来设白名单）
  - 限制最大文件 100 MB

#### 2.1.4 命令执行
- ❌ **不调用 `system()` / `popen()` / shell**
- 如必须调外部程序，用 `execvp` 数组形式

#### 2.1.5 文件权限
- 数据库文件：`0600`（只有 owner 可读写）
- 配置文件：`0600`
- 日志文件：`0600`

```cpp
chmod(db_path.c_str(), 0600);
```

### 2.2 ✅ 推荐做

#### 2.2.1 输入校验
- topic 名只允许 `[a-z0-9-]`
- 卡片 id 是合法整数
- markdown 文件大小上限

#### 2.2.2 资源限制
- 最大同时打开 fd 数
- 最大 SQL 查询超时（5s）
- LLM 请求超时（30s）

#### 2.2.3 日志脱敏
- 不打印用户答案完整内容（trace 级可以，info 不行）
- 不打印任何凭据

### 2.3 ⚠️ 不需要做（明确决定不做）

- ❌ 数据库加密 — 本地数据，OS 用户已隔离
- ❌ 双因素认证 — 单机工具
- ❌ 审计日志 — 不是合规场景

---

## 3. 第三方依赖安全

### 3.1 依赖审查
- 只用知名活跃维护的库
- 锁定版本（不要 `master` / `main`）
- 定期检查是否有 CVE
  ```bash
  # 用 OSV-Scanner
  osv-scanner --lockfile=cmake_deps.json
  ```

### 3.2 当前依赖审计

| 库              | 版本     | 维护者          | CVE        | 风险 |
|---------------|--------|--------------|------------|----|
| CLI11         | v2.4.2 | 知名社区        | 无          | 低 |
| spdlog        | v1.14  | gabime       | 无          | 低 |
| nlohmann/json | v3.11  | 知名         | 无          | 低 |
| toml++        | v3.4   | marzer       | 无          | 低 |
| SQLite3       | 系统     | 官方        | 看版本       | 低 |
| libcurl       | 系统     | 官方        | 看版本       | 中（要用 ≥ 8.x） |
| FTXUI         | TBD    | ArthurSonzogni | 无       | 低 |

---

## 4. 编译期安全

### 4.1 启用警告
```cmake
add_compile_options(
    -Wall -Wextra -Wpedantic -Werror
    -Wformat=2 -Wformat-security
    -Wnull-dereference
    -Wdouble-promotion
)
```

### 4.2 加固选项
```cmake
add_compile_options(
    -fstack-protector-strong
    -D_FORTIFY_SOURCE=2
    -fPIE
)
add_link_options(
    -pie
    -Wl,-z,relro,-z,now
    -Wl,-z,noexecstack
)
```

### 4.3 Sanitizers（CI 必跑）
- AddressSanitizer
- UndefinedBehaviorSanitizer
- ThreadSanitizer（独立 job）

---

## 5. 运行时安全

### 5.1 不信任用户输入
所有外部输入都校验：
- CLI 参数
- 配置文件
- markdown 内容
- LLM 响应

### 5.2 错误信息不泄密
- 不显示完整文件路径（只显示文件名）
- 不显示数据库内部错误细节
- 不 echo 用户输入到日志

### 5.3 临时文件
- 用 `/tmp` 时设 `0600`
- 程序退出时清理

---

## 6. 网络安全

### 6.1 HTTPS only
所有 LLM API 请求必须 HTTPS：
```cpp
if (!url.starts_with("https://")) {
    return Result<...>::err({E::kHttpInsecure, "must use https"});
}
```

### 6.2 证书校验
不要禁用证书校验：
```cpp
// ✓ 默认校验
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);

// ✗ 严禁
curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
```

### 6.3 超时
```cpp
curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5L);
```

---

## 7. 漏洞响应

### 7.1 报告渠道
- GitHub Security Advisory
- 邮箱：security@<project>.com（如有）

### 7.2 响应时间
- Critical: 24 小时内确认
- High: 72 小时内确认
- Other: 1 周内确认

### 7.3 修复流程
1. 在私有分支修复
2. 测试
3. 协调披露日期
4. 发 PATCH 版本
5. 公告 + CVE（如适用）

---

## 8. 自查清单（每次 PR 必检）

- [ ] 无硬编码密钥 / 凭据
- [ ] 所有 SQL 用 prepared statement
- [ ] 所有外部输入校验
- [ ] 不调 shell 命令
- [ ] 文件权限正确
- [ ] HTTPS only
- [ ] 错误信息无敏感信息
- [ ] 通过 sanitizer

---

## 9. 工具

### 9.1 静态分析
```bash
# clang-tidy
clang-tidy src/**/*.cpp -- -std=c++20

# cppcheck
cppcheck --enable=all --suppress=missingIncludeSystem src/

# Coverity（开源版）
```

### 9.2 动态分析
```bash
# 在 ASan 下跑测试
cmake -DCMAKE_BUILD_TYPE=Debug ..
ctest

# Valgrind
valgrind --leak-check=full ./build/src/bagu init
```

### 9.3 依赖检查
```bash
osv-scanner --lockfile=cmake_deps.json
```

---

## 10. 相关文档

- [错误处理](../operations/error-handling.md)
- [日志规范](../operations/logging.md)
- [架构总览](../architecture/overview.md)
