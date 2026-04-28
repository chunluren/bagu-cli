# 性能基准

> **状态**：Approved
> **更新**：2026-04-29

---

## 1. 性能目标

详见 [性能预算](../quality/performance-budget.md)。

核心指标：
- CLI 启动 < 50ms
- 搜索响应 < 100ms
- 导入 1000 cards/s

---

## 2. 工具

**Google Benchmark**（FetchContent 拉取）。

`tests/bench/CMakeLists.txt`：
```cmake
FetchContent_Declare(
    benchmark
    GIT_REPOSITORY https://github.com/google/benchmark.git
    GIT_TAG v1.8.5
)
set(BENCHMARK_ENABLE_TESTING OFF)
FetchContent_MakeAvailable(benchmark)

add_executable(bagu_bench
    sm2_bench.cpp
    parser_bench.cpp
    db_bench.cpp
    search_bench.cpp
)
target_link_libraries(bagu_bench PRIVATE benchmark::benchmark bagu_lib)
```

---

## 3. 基准用例

### 3.1 SM-2 算法

```cpp
#include <benchmark/benchmark.h>
#include "bagu/core/sm2.h"

static void BM_SM2_Update(benchmark::State& state) {
    bagu::core::ReviewState s{};
    int score = state.range(0);
    for (auto _ : state) {
        s = bagu::core::SM2Algorithm::update(s, score);
        benchmark::DoNotOptimize(s);
    }
}
BENCHMARK(BM_SM2_Update)->Arg(0)->Arg(3)->Arg(5);
```

**目标：** < 100 ns/op

### 3.2 Markdown 解析

```cpp
static void BM_Parser(benchmark::State& state) {
    std::string content = load_file("test_data/mysql-interview.md");
    bagu::parser::MarkdownParser parser;
    for (auto _ : state) {
        auto doc = parser.parse_string(content);
        benchmark::DoNotOptimize(doc);
    }
}
BENCHMARK(BM_Parser)->Unit(benchmark::kMillisecond);
```

**目标：** < 100 ms/file（10000 行 md）

### 3.3 SQLite 写入

```cpp
static void BM_CardInsertSingle(benchmark::State& state) {
    Database db(":memory:");
    db.migrate();
    CardDao dao(&db);

    for (auto _ : state) {
        Card c{.question = "q", .answer = "a", .topic_id = 1};
        dao.insert(c);
    }
}
BENCHMARK(BM_CardInsertSingle);

static void BM_CardInsertBatch(benchmark::State& state) {
    Database db(":memory:");
    db.migrate();
    CardDao dao(&db);

    int batch_size = state.range(0);
    for (auto _ : state) {
        auto txn = db.begin();
        for (int i = 0; i < batch_size; ++i) {
            Card c{.question = "q", .answer = "a", .topic_id = 1};
            dao.insert(c);
        }
        txn.commit();
    }
    state.SetItemsProcessed(state.iterations() * batch_size);
}
BENCHMARK(BM_CardInsertBatch)->Arg(100)->Arg(1000)->Arg(10000);
```

**目标：**
- 单条 insert：< 1 ms
- 批量（事务）：> 10000 cards/s

### 3.4 全文搜索

```cpp
static void BM_FullTextSearch(benchmark::State& state) {
    auto db = setup_db_with_n_cards(state.range(0));
    SearchEngine engine(db);

    for (auto _ : state) {
        auto results = engine.search("MVCC", std::nullopt, 20);
        benchmark::DoNotOptimize(results);
    }
}
BENCHMARK(BM_FullTextSearch)->Arg(1000)->Arg(10000)->Arg(100000)
    ->Unit(benchmark::kMillisecond);
```

**目标：** < 100 ms（10 万 cards）

### 3.5 启动时间

```bash
# 单独测，不算在 benchmark 内
hyperfine --warmup 3 --runs 50 './build/src/bagu --version'
```

**目标：** mean < 50 ms

---

## 4. 运行 benchmark

```bash
cd build
cmake --build . --target bagu_bench
./tests/bench/bagu_bench

# 输出 JSON 用于持续追踪
./tests/bench/bagu_bench --benchmark_format=json --benchmark_out=bench.json

# 仅跑某个
./tests/bench/bagu_bench --benchmark_filter=SM2
```

---

## 5. 结果记录

每次重大优化后更新 [PERFORMANCE.md](../../PERFORMANCE.md)（记录历次跑分）：

```markdown
## 2026-05-01 baseline
| Benchmark            | Time/op  | Throughput   |
|---------------------|----------|--------------|
| BM_SM2_Update       | 50 ns    | -            |
| BM_Parser           | 80 ms    | -            |
| BM_CardInsertBatch/1000 | 100 ms | 10000/s    |
| BM_FullTextSearch/10000 | 25 ms | -            |

## 2026-05-15 优化 SQLite 后
| Benchmark            | 改进                |
|---------------------|--------------------|
| BM_CardInsertBatch  | 100ms → 60ms（+66%）|
```

---

## 6. CI 集成

PR 跑 benchmark 比对，回退超 10% 报警：
```yaml
# .github/workflows/bench.yml
- name: Run benchmarks
  run: ./build/tests/bench/bagu_bench --benchmark_out=bench.json
- name: Compare with main
  uses: benchmark-action/github-action-benchmark@v1
  with:
    tool: 'googlecpp'
    output-file-path: bench.json
    alert-threshold: '110%'
```

---

## 7. Profiling

跑 benchmark 时同时采样：
```bash
perf record -g ./tests/bench/bagu_bench --benchmark_filter=BM_Parser
perf report
```

火焰图：
```bash
perf script | stackcollapse-perf.pl | flamegraph.pl > flame.svg
```

---

## 8. 已知性能瓶颈（持续追踪）

| 模块             | 当前瓶颈           | 计划优化                    |
|---------------|----------------|-------------------------|
| Markdown 解析   | cmark 较慢        | 切换 md4c                |
| 中文 FTS       | N-gram 索引大       | 引入 cppjieba            |
| SQLite 批量写  | 默认事务太频繁       | 加 WAL + 批量提交（已做）   |

---

## 9. 相关文档

- [性能预算](../quality/performance-budget.md)
- [测试策略](./strategy.md)
