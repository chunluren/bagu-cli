# 性能预算

> **状态**：Approved
> **更新**：2026-04-29

---

## 1. 总体目标

bagu-cli 是 **CLI 工具**，必须感觉"瞬间响应"。否则用户会改回 typora。

---

## 2. 性能预算（SLO）

| 操作                       | 目标          | 极限         | 测量方式                          |
|---------------------------|-------------|-------------|---------------------------------|
| `bagu --version`         | < 30 ms      | < 50 ms     | hyperfine                       |
| `bagu list` (5 主题)       | < 50 ms      | < 100 ms    | hyperfine                       |
| `bagu search "MVCC"` (5K cards) | < 100 ms | < 500 ms | hyperfine                       |
| `bagu search` (100K cards)| < 500 ms    | < 2 s       | hyperfine                       |
| `bagu import` 1 文件 1KB | < 50 ms      | < 200 ms    | benchmark                       |
| `bagu import` 1 文件 100KB | < 500 ms   | < 2 s       | benchmark                       |
| `bagu import` 100 文件   | < 5 s        | < 15 s      | benchmark                       |
| `bagu review` 启动到首屏 | < 200 ms     | < 500 ms    | 手工掐                          |
| `bagu review` 翻页响应   | < 50 ms      | < 100 ms    | 感受                            |
| `bagu interview` LLM 首字 | < 1 s       | < 3 s       | 网络相关                         |
| 单条 SQL 查询             | < 5 ms       | < 20 ms     | benchmark                       |
| 单条 SQL 写入（事务）       | < 10 ms      | < 50 ms     | benchmark                       |

---

## 3. 资源预算

| 资源              | 预算           | 警戒线          |
|----------------|------------|--------------|
| 运行内存（RSS）     | < 30 MB    | < 100 MB     |
| 数据库大小（普通用户）| < 50 MB    | < 500 MB     |
| 二进制大小（Release）| < 5 MB    | < 10 MB      |
| 二进制大小（静态链接） | < 15 MB   | < 30 MB      |
| 启动时 syscall 数 | < 100      | < 500        |
| FD 占用          | < 20       | < 100        |

---

## 4. 设计约束

为达成上述指标，设计上必须：

### 4.1 启动快
- **延迟初始化**：用不到的模块不加载
- **不读不必要的文件**：CLI 启动只 stat 配置文件
- **不连不必要的资源**：没用 LLM 命令时不初始化 HTTP 客户端

### 4.2 SQLite 优化
- **WAL 模式**：`PRAGMA journal_mode = WAL`
- **批量写用事务**：单条 fsync 极慢
- **prepared statement 复用**
- **必要的索引**（见 data-model.md）
- **FTS5 用 unicode61 tokenizer**

```cpp
db_->execute("PRAGMA journal_mode = WAL");
db_->execute("PRAGMA synchronous = NORMAL");
db_->execute("PRAGMA cache_size = -10000");   // 10MB cache
db_->execute("PRAGMA temp_store = MEMORY");
```

### 4.3 内存
- **避免不必要的拷贝**：传 const ref / 用 move
- **大对象用 unique_ptr / 引用**
- **TUI 不缓存全部 cards**：只缓存当前可见的

### 4.4 CPU
- **避免在主线程 IO**：未来 review 期间预加载下一题
- **正则用 RE2，不要标准库 regex**（前者 10x 快）

---

## 5. 测量方法

### 5.1 自动化（CI）
每次 PR 跑 [benchmark](../testing/benchmarks.md)，回退超 10% 失败。

### 5.2 手工
```bash
# 启动时间
hyperfine --warmup 3 --runs 50 './build/src/bagu --version'

# 内存
/usr/bin/time -v ./build/src/bagu list

# syscall
strace -c ./build/src/bagu --version
```

### 5.3 持续监控
项目根 `PERFORMANCE.md` 记录每次跑分历史。

---

## 6. 性能预算超标处理

CI 检测到超标会失败。处理流程：

1. **Profiling** — perf / strace / 内存分析
2. **判断**：
   - 是 bug → 修
   - 是合理增长 → 调整预算（要 ADR）
   - 是非关键增长但可优化 → 优化或暂时调宽
3. **更新 PERFORMANCE.md**
4. **回归测试**

---

## 7. 历史性能记录（PERFORMANCE.md 摘要）

详见 `PERFORMANCE.md`（项目根，每次大版本更新）。

---

## 8. 反模式

❌ 启动时连数据库 schema 都不需要的命令也连
❌ 单条 INSERT 走 fsync（1000 条 = 1 秒）
❌ TUI 一次性加载所有 cards
❌ 用 std::regex
❌ 用 `system("xxx")` 调用外部命令
❌ 同步阻塞调用 LLM API（block 整个进程）

---

## 9. 相关文档

- [基准测试](../testing/benchmarks.md)
- [数据模型](../architecture/data-model.md)
