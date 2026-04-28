# 测试策略

> **状态**：Approved
> **更新**：2026-04-29

---

## 1. 测试金字塔

```
         /\
        /e2e\           少量（关键命令链路）
       /------\
      /集成测试\          中量（service + db）
     /----------\
    /  单元测试   \      大量（纯函数、领域逻辑）
   /--------------\
```

| 层级    | 占比      | 工具          | 跑的频率           |
|-------|---------|-------------|---------------|
| 单元测试  | 70%     | GoogleTest  | 每次提交、每次 PR    |
| 集成测试  | 20%     | GoogleTest + 真实 SQLite | 每次 PR + nightly |
| e2e 测试 | 10%     | bash + bagu | 每次 PR + nightly |

---

## 2. 覆盖率目标

| 模块           | 目标覆盖率 |
|--------------|-------|
| core（领域逻辑） | ≥ 95% |
| db / parser  | ≥ 85% |
| service      | ≥ 80% |
| util         | ≥ 80% |
| cli / tui    | ≥ 50%（UI 层难测）|
| llm          | ≥ 60%（mock）|
| **整体**       | ≥ 80% |

---

## 3. 单元测试

### 3.1 原则
- **纯函数优先** — core 模块尽量纯
- **小而快** — 单测 ≤ 100ms
- **独立** — 不依赖外部资源 / 测试顺序
- **可读** — Arrange-Act-Assert 三段
- **覆盖边界** — 0 / 1 / N、空 / 满、最大 / 最小

### 3.2 命名

```cpp
TEST(<被测类>, <场景_预期>)
```

例：
```cpp
TEST(SM2Algorithm, UpdateWithScore5_IncreasesInterval)
TEST(SM2Algorithm, UpdateWithScore0_ResetsRepetitions)
TEST(CardDao, FindById_NotFound_ReturnsError)
```

### 3.3 模板

```cpp
#include <gtest/gtest.h>
#include "bagu/core/sm2.h"

namespace bagu::core {

TEST(SM2Algorithm, UpdateWithScore5_FirstReview_IntervalEqualsOne) {
    // Arrange
    ReviewState s{};

    // Act
    auto result = SM2Algorithm::update(s, 5);

    // Assert
    EXPECT_EQ(result.interval_days, 1);
    EXPECT_EQ(result.repetitions, 1);
    EXPECT_GT(result.ease_factor, 2.5);
}

TEST(SM2Algorithm, UpdateWithScore0_AnyState_ResetsToInitial) {
    ReviewState s{.interval_days = 30, .ease_factor = 2.7, .repetitions = 5};

    auto result = SM2Algorithm::update(s, 0);

    EXPECT_EQ(result.interval_days, 1);
    EXPECT_EQ(result.repetitions, 0);
    // ease_factor 应该减小但不低于 1.3
    EXPECT_LT(result.ease_factor, s.ease_factor);
    EXPECT_GE(result.ease_factor, 1.3);
}

}  // namespace bagu::core
```

### 3.4 文件组织

```
tests/
├── CMakeLists.txt
├── unit/
│   ├── core/
│   │   ├── sm2_test.cpp
│   │   └── card_picker_test.cpp
│   ├── db/
│   │   └── card_dao_test.cpp
│   ├── parser/
│   │   └── markdown_parser_test.cpp
│   └── util/
│       ├── string_test.cpp
│       └── time_test.cpp
├── integration/
│   ├── import_test.cpp
│   ├── review_flow_test.cpp
│   └── search_test.cpp
└── e2e/
    ├── basic_commands.sh
    └── full_workflow.sh
```

### 3.5 测试夹具

```cpp
class CardDaoTest : public ::testing::Test {
protected:
    void SetUp() override {
        db_ = std::make_unique<Database>(":memory:");
        db_->migrate();
        dao_ = std::make_unique<CardDao>(db_.get());
    }

    std::unique_ptr<Database> db_;
    std::unique_ptr<CardDao> dao_;
};

TEST_F(CardDaoTest, Insert_ReturnsValidId) {
    Card c{.question = "q", .answer = "a", .topic_id = 1};
    auto id = dao_->insert(c);
    EXPECT_GT(id.value(), 0);
}
```

### 3.6 参数化测试

```cpp
class SM2ScoreTest : public ::testing::TestWithParam<int> {};

TEST_P(SM2ScoreTest, ScoreInValidRange_AlwaysReturnsValidState) {
    int score = GetParam();
    ReviewState s{};
    auto result = SM2Algorithm::update(s, score);

    EXPECT_GE(result.ease_factor, 1.3);
    EXPECT_GE(result.interval_days, 0);
}

INSTANTIATE_TEST_SUITE_P(AllScores, SM2ScoreTest, ::testing::Range(0, 6));
```

---

## 4. 集成测试

### 4.1 范围
- service + db 联动
- 真实 SQLite（内存或临时文件）
- 不调外部网络

### 4.2 模板

```cpp
class ImportServiceIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 用临时数据库
        tmp_db_ = "/tmp/bagu-test-" + std::to_string(getpid()) + ".db";
        db_ = std::make_shared<Database>(tmp_db_);
        db_->migrate();
        svc_ = std::make_unique<ImportService>(db_);
    }

    void TearDown() override {
        std::filesystem::remove(tmp_db_);
    }

    std::string tmp_db_;
    std::shared_ptr<Database> db_;
    std::unique_ptr<ImportService> svc_;
};

TEST_F(ImportServiceIntegrationTest, ImportMarkdownFile_CreatesTopicAndCards) {
    // 准备一个测试 md
    std::string md = "# Test\n\n## 第 1 章\n\n**Q1. 问** 答\n";
    auto path = create_temp_md(md);

    auto result = svc_->import(path);

    EXPECT_TRUE(result.is_ok());
    EXPECT_EQ(result.value().card_count, 1);

    // 验证入库
    auto cards = CardDao(db_.get()).find_all();
    ASSERT_EQ(cards.value().size(), 1);
    EXPECT_EQ(cards.value()[0].question, "问");
}
```

---

## 5. e2e 测试

### 5.1 范围
- 完整命令调用
- 验证退出码 / stdout / stderr
- 验证数据库状态

### 5.2 模板（bash）

```bash
#!/usr/bin/env bash
# tests/e2e/basic_commands.sh
set -e

BAGU=./build/src/bagu
TMPDIR=$(mktemp -d)
export BAGU_HOME=$TMPDIR

cleanup() { rm -rf $TMPDIR; }
trap cleanup EXIT

# Test 1: init
echo "Test 1: bagu init"
$BAGU init
[[ -f $TMPDIR/bagu.db ]] || { echo "FAIL: db not created"; exit 1; }
echo "  ✓ PASS"

# Test 2: import
echo "Test 2: bagu import"
mkdir $TMPDIR/docs
cat > $TMPDIR/docs/test.md <<'EOF'
# Test Topic
## 第 1 章 测试
**Q1. 问题？** 答案
EOF
$BAGU import $TMPDIR/docs/
echo "  ✓ PASS"

# Test 3: search
echo "Test 3: bagu search"
output=$($BAGU search "问题")
[[ $output == *"答案"* ]] || { echo "FAIL: search not finding"; exit 1; }
echo "  ✓ PASS"

echo "All e2e tests passed!"
```

---

## 6. Mock 与隔离

### 6.1 LLM 客户端 mock

```cpp
class MockLLMClient : public LLMClient {
public:
    MOCK_METHOD(std::string, chat, (const ChatRequest&), (override));
    MOCK_METHOD(void, chat_stream,
        (const ChatRequest&, std::function<void(const std::string&)>),
        (override));
};

TEST(InterviewService, AskQuestion_CallsLLM) {
    auto mock = std::make_shared<MockLLMClient>();
    EXPECT_CALL(*mock, chat(::testing::_))
        .WillOnce(::testing::Return("Mock question"));

    InterviewService svc(mock, db);
    auto q = svc.next_question("mysql");
    EXPECT_EQ(q, "Mock question");
}
```

### 6.2 时间隔离

```cpp
// 不要直接用 time(nullptr)
// 用 Clock 抽象
class Clock {
public:
    virtual int64_t now() = 0;
};

class SystemClock : public Clock { ... };
class FakeClock : public Clock { /* 测试时设固定值 */ };
```

---

## 7. 性能测试

详见 [benchmarks.md](./benchmarks.md)。

用 Google Benchmark：
```cpp
#include <benchmark/benchmark.h>

static void BM_CardDaoInsert(benchmark::State& state) {
    Database db(":memory:");
    db.migrate();
    CardDao dao(&db);

    for (auto _ : state) {
        Card c{.question = "q", .answer = "a", .topic_id = 1};
        dao.insert(c);
    }
}
BENCHMARK(BM_CardDaoInsert);
```

---

## 8. 持续集成

详见 [ci-cd.md](../operations/ci-cd.md)。

PR 必跑：
- 编译（GCC + Clang）
- 单元测试
- 集成测试
- e2e 测试
- 覆盖率（不达标失败）
- clang-format 检查

---

## 9. 测试反模式

❌ **测试调用顺序** — 单测必须独立
❌ **测试私有方法** — 测试公开行为
❌ **mock 太多** — 比例失衡说明设计有问题
❌ **断言无意义** — `EXPECT_TRUE(true)`、`EXPECT_EQ(1, 1)` 不算测试
❌ **跨测试共享状态** — 用 SetUp/TearDown 隔离
❌ **慢测试** — 单测 > 1s 必须优化
❌ **跳过失败的测试** — 修，别跳

---

## 10. 测试检查清单

代码 review 时看测试：
- [ ] 新功能必须有测试
- [ ] 修 bug 必须先有失败测试再修
- [ ] 测试名清晰描述场景与预期
- [ ] 覆盖正常 + 异常 + 边界
- [ ] 可独立运行
- [ ] 跑得快

---

## 11. 相关文档

- [GoogleTest 文档](https://google.github.io/googletest/)
- [Google Benchmark 文档](https://github.com/google/benchmark)
- [基准测试](./benchmarks.md)
- [CI/CD](../operations/ci-cd.md)
