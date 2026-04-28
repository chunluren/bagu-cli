# 模块详细设计（LLD）

> **状态**：Approved
> **版本**：v1.0
> **更新**：2026-04-29

---

## 1. 模块全景

| 模块      | 路径           | 职责                             |
|---------|--------------|--------------------------------|
| cli     | src/cli/     | 命令解析、子命令分发                  |
| service | src/service/ | 应用层服务（编排）                    |
| core    | src/core/    | 领域逻辑（SM-2、评分、调度）            |
| db      | src/db/      | SQLite 封装、DAO                  |
| parser  | src/parser/  | Markdown 解析                    |
| search  | src/search/  | FTS5 全文搜索                     |
| llm     | src/llm/     | LLM API 客户端                    |
| tui     | src/tui/     | FTXUI 交互界面                    |
| util    | src/util/    | 通用工具                          |

---

## 2. cli 模块

### 2.1 职责
- 用 CLI11 解析命令行参数
- 把命令分发到对应 service
- 格式化输出（彩色 / JSON）

### 2.2 文件结构
```
src/cli/
├── command.h           # CommandHandler 抽象基类
├── command_init.cpp
├── command_import.cpp
├── command_list.cpp
├── command_search.cpp
├── command_review.cpp
├── command_interview.cpp
├── command_stats.cpp
├── command_config.cpp
├── command_show.cpp
├── output.h            # 彩色输出 / JSON 格式化
└── output.cpp
```

### 2.3 关键类

```cpp
// 抽象基类
class CommandHandler {
public:
    virtual ~CommandHandler() = default;
    virtual int execute(const CommandContext& ctx) = 0;
};

// 上下文
struct CommandContext {
    std::shared_ptr<Database> db;
    std::shared_ptr<Config> cfg;
    bool verbose;
    bool json_output;
    bool no_color;
};
```

---

## 3. service 模块

### 3.1 职责
应用层服务，编排领域逻辑和基础设施。

### 3.2 文件结构
```
src/service/
├── import_service.{h,cpp}      # 导入流程
├── review_service.{h,cpp}      # 复习流程
├── search_service.{h,cpp}      # 搜索流程
├── interview_service.{h,cpp}   # 面试流程
└── stats_service.{h,cpp}       # 统计
```

### 3.3 关键接口

```cpp
class ReviewService {
public:
    ReviewService(std::shared_ptr<Database> db,
                  std::shared_ptr<SM2Algorithm> algo);

    // 取今日待复习的卡片
    std::vector<Card> get_due_cards(
        const std::optional<std::string>& topic,
        int limit);

    // 提交评分
    void submit_review(int64_t card_id, int score, int duration_ms);

private:
    std::shared_ptr<Database> db_;
    std::shared_ptr<SM2Algorithm> algo_;
};
```

---

## 4. core 模块

### 4.1 职责
纯领域逻辑，无 IO，**全是纯函数 / 无副作用类**，便于测试。

### 4.2 文件结构
```
src/core/
├── sm2.{h,cpp}             # SM-2 算法
├── card_picker.{h,cpp}     # 卡片选择策略
├── scorer.{h,cpp}          # 评分系统（接 LLM 用）
└── models.h                # Card / Review 等 POD 数据结构
```

### 4.3 SM-2 算法

```cpp
namespace bagu::core {

struct ReviewState {
    int interval_days = 0;
    double ease_factor = 2.5;
    int repetitions = 0;
};

class SM2Algorithm {
public:
    /// 根据评分（0-5）更新 ReviewState
    /// score 0-2: 失败 → repetitions 重置，间隔=1
    /// score 3-5: 成功 → 间隔指数增长，ease 调整
    static ReviewState update(ReviewState s, int score);

    /// 计算下次复习时间（unix timestamp）
    static int64_t next_review_time(const ReviewState& s, int64_t now_ts);
};

} // namespace bagu::core
```

### 4.4 测试要求
- 100% 路径覆盖
- 边界值（score=0, score=5, repetitions=0/1/MAX）

---

## 5. db 模块

### 5.1 职责
- SQLite 连接管理
- DAO（数据访问对象）封装
- 事务管理
- Schema migration

### 5.2 文件结构
```
src/db/
├── database.{h,cpp}        # SQLite 连接 / 事务 / 迁移
├── topic_dao.{h,cpp}
├── chapter_dao.{h,cpp}
├── card_dao.{h,cpp}
├── review_dao.{h,cpp}
├── interview_dao.{h,cpp}
└── migrations/
    ├── 0001_initial.sql
    ├── 0002_add_xxx.sql
    └── ...
```

### 5.3 关键类

```cpp
class Database {
public:
    explicit Database(const std::string& path);
    ~Database();

    void migrate();                 // 应用未执行的迁移
    int get_schema_version() const;

    // 事务
    Transaction begin();

    // 给 DAO 用
    sqlite3* handle() { return db_; }

private:
    sqlite3* db_;
};

class Transaction {
public:
    Transaction(sqlite3* db);
    ~Transaction();         // 析构时如未 commit 则 rollback

    void commit();
    void rollback();

private:
    sqlite3* db_;
    bool committed_ = false;
};
```

### 5.4 错误处理

所有 DB 操作返回 `Result<T>`：

```cpp
template <typename T>
class Result {
public:
    bool ok() const;
    T& value();
    const Error& error() const;
};

// 使用
auto cards = card_dao.find_due(topic, 10);
if (!cards.ok()) {
    spdlog::error("查询失败：{}", cards.error().message);
    return -1;
}
for (auto& c : cards.value()) { ... }
```

---

## 6. parser 模块

### 6.1 职责
解析 markdown 文档，抽取 topic / chapter / card。

### 6.2 文件结构
```
src/parser/
├── markdown_parser.{h,cpp}     # cmark 封装
├── card_extractor.{h,cpp}      # AST → Card
├── topic_metadata.{h,cpp}      # 从文件名 / front matter 推 topic
└── tests/
```

### 6.3 抽取规则

```cpp
struct ParsedDocument {
    Topic topic;
    std::vector<Chapter> chapters;
    std::vector<Card> cards;
};

class MarkdownParser {
public:
    ParsedDocument parse(const std::string& file_path);

private:
    // 规则 1：## 第 N 章 XXX → Chapter
    void extract_chapter(cmark_node* heading);

    // 规则 2：### N.M XXX → 子 Chapter，正文作为 Card
    void extract_section_card(cmark_node* heading);

    // 规则 3：**Q数字. 问题** 答案 → Card
    void extract_qa_cards(cmark_node* paragraph);
};
```

### 6.4 边界情况
- 无标题的 md → 当作单一 Card
- 嵌套标题 → 用 parent_id 维护层级
- 代码块内的 `**Q1.**` → 不抽取（避免误识别）

---

## 7. search 模块

### 7.1 职责
基于 SQLite FTS5 的全文搜索。

### 7.2 文件结构
```
src/search/
├── search_engine.{h,cpp}
├── tokenizer.{h,cpp}        # 中文分词（N-gram 或 cppjieba）
└── highlighter.{h,cpp}      # 关键词高亮
```

### 7.3 关键 API

```cpp
class SearchEngine {
public:
    explicit SearchEngine(std::shared_ptr<Database> db);

    struct SearchResult {
        Card card;
        double rank;
        std::vector<std::pair<int, int>> highlights;   // 高亮区间
    };

    std::vector<SearchResult> search(
        const std::string& query,
        const std::optional<std::string>& topic = std::nullopt,
        int limit = 20);
};
```

### 7.4 中文分词
- v0.1：用 N-gram（每 2 字组合）作为简单分词
- v0.2：考虑接 cppjieba

---

## 8. llm 模块

### 8.1 职责
封装 LLM API（OpenAI / Claude / Ollama）。

### 8.2 文件结构
```
src/llm/
├── llm_client.h             # 抽象基类
├── openai_client.{h,cpp}
├── claude_client.{h,cpp}
├── ollama_client.{h,cpp}
└── prompt_templates.{h,cpp} # prompt 模板
```

### 8.3 抽象接口

```cpp
class LLMClient {
public:
    struct Message {
        std::string role;       // "system" / "user" / "assistant"
        std::string content;
    };

    struct ChatRequest {
        std::vector<Message> messages;
        std::string model;
        double temperature = 0.7;
        int max_tokens = 1000;
        bool stream = false;
    };

    virtual ~LLMClient() = default;

    // 同步（非流式）
    virtual std::string chat(const ChatRequest& req) = 0;

    // 流式
    virtual void chat_stream(
        const ChatRequest& req,
        std::function<void(const std::string&)> on_chunk) = 0;
};
```

### 8.4 Prompt 模板

```cpp
namespace prompts {

constexpr const char* INTERVIEW_SYSTEM = R"(
你是一名资深 C++ 后端面试官。请围绕主题 [{topic}] 提问。
要求：
1. 一次只问一个问题
2. 用户回答后给 0-10 分评分和具体改进建议
3. 根据用户表现自适应调整下一题难度
)";

constexpr const char* INTERVIEW_QUESTION = R"(
当前对话历史：
{history}

请提下一个问题。难度提示：{difficulty_hint}
)";

constexpr const char* INTERVIEW_GRADE = R"(
问题：{question}
用户答案：{answer}

请：
1. 评分（0-10）
2. 列出答对的点
3. 列出遗漏 / 错误的点
4. 给出 1-2 句建议
)";

} // namespace prompts
```

---

## 9. tui 模块

### 9.1 职责
基于 FTXUI 的终端交互界面。

### 9.2 文件结构
```
src/tui/
├── review_screen.{h,cpp}       # 复习界面
├── interview_screen.{h,cpp}    # 面试界面
├── components/
│   ├── card_view.cpp
│   ├── score_buttons.cpp
│   └── progress_bar.cpp
└── theme.{h,cpp}              # 颜色主题
```

### 9.3 复习界面骨架

```cpp
class ReviewScreen {
public:
    ReviewScreen(std::shared_ptr<ReviewService> svc);

    int run();    // 返回退出码

private:
    ftxui::Component build_root();
    void on_score(int score);
    void next_card();

    std::shared_ptr<ReviewService> svc_;
    std::vector<Card> cards_;
    size_t current_ = 0;
    bool answer_revealed_ = false;
};
```

---

## 10. util 模块

### 10.1 职责
通用工具，无业务逻辑。

### 10.2 文件结构
```
src/util/
├── string.{h,cpp}      # split / trim / contains
├── time.{h,cpp}        # 时间戳 / 格式化
├── file.{h,cpp}        # 文件读写 / 路径
├── hash.{h,cpp}        # SHA256
├── http.{h,cpp}        # libcurl 封装
└── result.h            # Result<T>、Error
```

### 10.3 Result 类型

```cpp
struct Error {
    int code;
    std::string message;
    std::string detail;
};

template <typename T>
class Result {
public:
    static Result ok(T value);
    static Result err(Error error);

    bool is_ok() const noexcept;
    bool is_err() const noexcept;

    T& value();
    const T& value() const;
    const Error& error() const;

    // monadic
    template <typename F>
    auto map(F&& f) -> Result<decltype(f(std::declval<T>()))>;

    template <typename F>
    auto and_then(F&& f);

private:
    std::variant<T, Error> data_;
};
```

---

## 11. 模块依赖图

```
        ┌─────────────────────────┐
        │           cli            │
        └───────────┬─────────────┘
                    │
                    ▼
        ┌─────────────────────────┐
        │         service          │
        └─┬─────┬─────┬─────┬───┬─┘
          │     │     │     │   │
          ▼     ▼     ▼     ▼   ▼
        ┌────┐┌──────┐┌─────┐┌────┐┌────┐
        │core││parser││search││llm ││ tui│
        └─┬──┘└──┬───┘└──┬──┘└─┬──┘└─┬──┘
          │     │       │     │     │
          └─────┴───┬───┴─────┴─────┘
                    ▼
                 ┌─────┐
                 │ db  │
                 └──┬──┘
                    ▼
                 ┌─────┐
                 │util │
                 └─────┘
```

**约束：**
- 同层不能互相调用
- 上层只能调下层
- util / db 不能调上层

---

## 12. 编码约定

详见 [编码规范](../development/coding-standards.md)。

每个模块都要：
- ✅ 头文件用 `#pragma once`
- ✅ 命名空间 `bagu::<module>`
- ✅ 公开类有 doxygen 注释
- ✅ 单元测试覆盖 ≥ 80%
- ✅ 不依赖未声明的全局状态

---

## 13. 相关文档

- [架构总览](./overview.md)
- [数据模型](./data-model.md)
- [CLI 规范](./cli-spec.md)
- [测试策略](../testing/strategy.md)
