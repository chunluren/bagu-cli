# 调试指南

> **状态**：Approved
> **更新**：2026-04-29

---

## 1. Debug 构建

```bash
mkdir build-debug && cd build-debug
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
ninja
```

Debug 构建自动启用：
- `-O0 -g`
- AddressSanitizer
- Assertions

---

## 2. GDB

### 2.1 基本用法
```bash
gdb ./src/bagu
(gdb) run review --topic mysql
(gdb) bt              # 崩溃后查栈
(gdb) frame 3         # 切到第 3 帧
(gdb) print var       # 看变量
(gdb) list            # 看上下文代码
```

### 2.2 设断点
```bash
(gdb) b main
(gdb) b CardDao::find_by_id
(gdb) b card_dao.cpp:42
(gdb) b src/db/card_dao.cpp:42 if id == 100
```

### 2.3 调试运行中的进程
```bash
gdb -p $(pgrep bagu)
```

### 2.4 .gdbinit 推荐
```
set print pretty on
set print array on
set pagination off
```

---

## 3. Sanitizers

### 3.1 AddressSanitizer (ASan)
默认 Debug 启用。捕获：
- 堆 / 栈 / 全局缓冲区溢出
- 释放后使用 (UAF)
- 二次释放
- 内存泄漏

### 3.2 ThreadSanitizer (TSan)
检测数据竞争。需要单独构建：
```bash
cmake -DCMAKE_BUILD_TYPE=Debug -DBAGU_TSAN=ON ..
```

### 3.3 UndefinedBehaviorSanitizer (UBSan)
捕获未定义行为：
```bash
add_compile_options(-fsanitize=undefined)
add_link_options(-fsanitize=undefined)
```

### 3.4 MemorySanitizer (MSan)
未初始化内存（仅 Clang）。

### 3.5 同时启用
```cmake
target_compile_options(bagu PRIVATE -fsanitize=address,undefined)
target_link_options(bagu PRIVATE -fsanitize=address,undefined)
```

---

## 4. Valgrind

适合无 ASan 的环境（如老编译器）：
```bash
valgrind --leak-check=full ./src/bagu list

# 检测数据竞争
valgrind --tool=helgrind ./src/bagu review

# 性能分析
valgrind --tool=callgrind ./src/bagu import /tmp/docs/
kcachegrind callgrind.out.<pid>
```

---

## 5. 性能分析

### 5.1 perf

```bash
# 采样
perf record -F 99 -g ./src/bagu import /tmp/docs/

# 报告
perf report

# 火焰图
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
```

### 5.2 strace

```bash
# 看系统调用
strace -c ./src/bagu list                 # 统计
strace -e trace=openat ./src/bagu init    # 只看文件打开
```

### 5.3 ltrace

```bash
ltrace -c ./src/bagu list   # 库函数调用统计
```

### 5.4 时间测量

CLI 工具简单跑：
```bash
time ./src/bagu search "MVCC"
```

---

## 6. SQLite 调试

### 6.1 打开数据库
```bash
sqlite3 ~/.bagu/bagu.db

sqlite> .schema                         # 看 schema
sqlite> .tables                         # 列表
sqlite> .headers on
sqlite> .mode column
sqlite> SELECT * FROM card LIMIT 5;
```

### 6.2 EXPLAIN QUERY PLAN
```sql
EXPLAIN QUERY PLAN
SELECT * FROM card_fts WHERE card_fts MATCH 'MVCC';
```

### 6.3 启用 SQL 日志
```cpp
// 调试时在 db_open 后启用
sqlite3_trace_v2(db_, SQLITE_TRACE_STMT,
    [](unsigned, void*, void* p, void* x) {
        spdlog::debug("SQL: {}", (char*)x);
        return 0;
    }, nullptr);
```

---

## 7. 日志调试

### 7.1 调高日志级别
```bash
./src/bagu --verbose review
# 或
SPDLOG_LEVEL=debug ./src/bagu review
```

### 7.2 在代码中
```cpp
spdlog::debug("loaded {} cards for topic {}", cards.size(), topic);
spdlog::trace("card_id={} ease={} interval={}", id, ease, interval);
```

### 7.3 临时打印
```cpp
// 临时调试用
fmt::print(stderr, ">>> here, x={}\n", x);
```

提交前必须清理。

---

## 8. TUI 调试

FTXUI 的问题是日志会被 TUI 覆盖。

**方案 1：日志重定向**
```cpp
// 启动 TUI 前
auto file_logger = spdlog::basic_logger_mt("file", "/tmp/bagu-debug.log");
spdlog::set_default_logger(file_logger);

// 另一个终端
tail -f /tmp/bagu-debug.log
```

**方案 2：tmux 分屏**
- 上半 tmux 跑 bagu review
- 下半 tail -f log

---

## 9. 常见问题排查

### Q1. 段错误
1. Debug 构建 + ASan
2. gdb 跑 → bt 看栈

### Q2. 内存泄漏
ASan 退出时自动报，或：
```bash
valgrind --leak-check=full ./src/bagu ...
```

### Q3. 数据库锁住
- 看是不是有未提交事务
- `lsof ~/.bagu/bagu.db` 看哪个进程占用

### Q4. CPU 占用高
- perf 采样火焰图
- 找热点函数优化

### Q5. 启动慢
```bash
strace -c ./src/bagu --version   # 看 syscall 时间分布
```

---

## 10. 测试驱动调试（TDD-style debug）

发现 bug 时：
1. **先写一个失败的单元测试** 复现问题
2. 修代码让测试通过
3. 永远保留这个测试，防回归

---

## 11. Core Dump

启用 core 文件：
```bash
ulimit -c unlimited
echo "/tmp/core_%e_%p" | sudo tee /proc/sys/kernel/core_pattern
```

崩溃后：
```bash
gdb ./src/bagu /tmp/core_bagu_12345
(gdb) bt full
```

---

## 12. 相关文档

- [开发环境](./setup.md)
- [测试策略](../testing/strategy.md)
- [日志规范](../operations/logging.md)
