# bagu-cli 架构设计

## 一、需求分解

### 核心场景

1. **导入** — 解析 markdown 八股文档，抽取知识点入库
2. **浏览** — 列出主题/章节，按需查看
3. **搜索** — 模糊查找包含关键词的所有题目
4. **复习** — 基于算法推送今日待复习题，评分后更新
5. **AI 面试** — 调 LLM 提问，AI 评分用户答案
6. **统计** — 进度、热力图、薄弱点分析

### 非功能需求

- **启动 < 100ms**（CLI 工具基本要求）
- **零外部依赖**（除 LLM API 外不联网）
- **跨平台**（Linux 优先，Mac 次之）
- **数据本地存储**（隐私安全）

---

## 二、整体架构

```
┌─────────────────────────────────────────────────┐
│              CLI / TUI 层（用户交互）             │
│   ┌─────────┐  ┌─────────┐  ┌─────────────┐    │
│   │ CLI11   │  │ FTXUI   │  │ 子命令分发    │    │
│   │ 参数解析 │  │ 交互界面 │  │ (dispatch)  │    │
│   └─────────┘  └─────────┘  └─────────────┘    │
└─────────────────────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────┐
│              核心业务层（Core）                   │
│   ┌──────────┐ ┌──────────┐ ┌──────────────┐   │
│   │ 复习算法 │ │ 评分系统 │ │ 进度统计     │   │
│   │ (SM-2)   │ │          │ │              │   │
│   └──────────┘ └──────────┘ └──────────────┘   │
└─────────────────────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────┐
│              服务层                              │
│   ┌────────┐ ┌──────────┐ ┌────────┐ ┌──────┐  │
│   │ 解析器 │ │ 搜索引擎 │ │ LLM API │ │ 配置 │  │
│   │ cmark  │ │ 倒排索引 │ │ libcurl │ │ TOML │  │
│   └────────┘ └──────────┘ └────────┘ └──────┘  │
└─────────────────────────────────────────────────┘
                       ↓
┌─────────────────────────────────────────────────┐
│              数据持久层                           │
│   ┌──────────┐  ┌──────────┐  ┌────────────┐   │
│   │ SQLite   │  │ 文件系统 │  │ 缓存目录    │   │
│   │ 元数据    │  │ 原 .md   │  │ ~/.bagu/   │   │
│   └──────────┘  └──────────┘  └────────────┘   │
└─────────────────────────────────────────────────┘
```

---

## 三、数据模型

### SQLite Schema

```sql
-- 主题（如 MySQL、Redis）
CREATE TABLE topic (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT NOT NULL UNIQUE,        -- "mysql"
    title       TEXT NOT NULL,                -- "MySQL 八股文档"
    file_path   TEXT NOT NULL,                -- 原 md 文件路径
    imported_at INTEGER NOT NULL              -- unix timestamp
);

-- 章节（如 "第3章 索引"）
CREATE TABLE chapter (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    topic_id    INTEGER NOT NULL,
    name        TEXT NOT NULL,                -- "索引"
    chapter_no  INTEGER,                      -- 3
    FOREIGN KEY (topic_id) REFERENCES topic(id)
);

-- 题目（最小单元，可以是问答对、知识点）
CREATE TABLE card (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    topic_id        INTEGER NOT NULL,
    chapter_id      INTEGER,
    question        TEXT NOT NULL,
    answer          TEXT NOT NULL,
    difficulty      INTEGER DEFAULT 2,        -- 1-3 (易/中/难)
    tags            TEXT,                     -- JSON array, e.g. ["mysql","index","B+树"]
    source_line     INTEGER,                  -- 在原 md 文件中的行号
    FOREIGN KEY (topic_id) REFERENCES topic(id),
    FOREIGN KEY (chapter_id) REFERENCES chapter(id)
);

-- 复习记录（SM-2 算法所需）
CREATE TABLE review (
    card_id         INTEGER PRIMARY KEY,
    last_review     INTEGER,                  -- 上次复习时间
    next_review     INTEGER,                  -- 下次复习时间
    interval_days   INTEGER DEFAULT 1,        -- 当前间隔
    ease_factor     REAL DEFAULT 2.5,         -- SM-2 难度因子
    review_count    INTEGER DEFAULT 0,
    correct_count   INTEGER DEFAULT 0,
    FOREIGN KEY (card_id) REFERENCES card(id)
);

-- 复习历史（用于热力图、统计）
CREATE TABLE review_history (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    card_id     INTEGER NOT NULL,
    reviewed_at INTEGER NOT NULL,
    score       INTEGER NOT NULL,             -- 0-5（SM-2 评分）
    FOREIGN KEY (card_id) REFERENCES card(id)
);

-- 倒排索引（全文搜索）
CREATE VIRTUAL TABLE card_fts USING fts5(
    question, answer, tags,
    content='card', content_rowid='id'
);

-- AI 面试会话
CREATE TABLE interview_session (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    topic       TEXT,
    started_at  INTEGER,
    ended_at    INTEGER,
    score       REAL,
    transcript  TEXT                          -- JSON
);
```

---

## 四、模块详解

### 4.1 Markdown 解析器（src/parser/）

**职责：** 把 markdown 八股文档拆成 card（题目）。

**策略：**

我们的八股文档结构是：
```
## 第 X 章 ZZZ
### X.1 小节
### X.2 ...
...
## 第 11 章 面试题库
### 11.1 类别
**Q1. 问题？** 答案...
**Q2. 问题？** 答案...
```

**两种 card 来源：**

1. **题库章节** — `**Q数字. 问题** 答案` 直接抽取（最高质量）
2. **正文小节** — 整个三级标题作为一个 card（小节标题 = question，正文 = answer）

**实现：** 用 cmark 解析为 AST，遍历提取。

### 4.2 复习算法（src/core/sm2.cpp）

**SM-2 算法**（SuperMemo 经典算法，Anki 也用）：

```cpp
struct ReviewState {
    int interval_days;    // 当前复习间隔
    double ease_factor;   // 难度因子（默认 2.5）
    int repetitions;      // 连续答对次数
};

// 评分 0-5：
// 5: 完美（不假思索）
// 4: 答对，稍有犹豫
// 3: 答对，需思考
// 2: 答错，但记起来了
// 1: 答错，模糊印象
// 0: 完全忘记
ReviewState update(ReviewState s, int score) {
    if (score < 3) {
        s.repetitions = 0;
        s.interval_days = 1;
    } else {
        if (s.repetitions == 0) s.interval_days = 1;
        else if (s.repetitions == 1) s.interval_days = 6;
        else s.interval_days = round(s.interval_days * s.ease_factor);
        s.repetitions++;
    }
    s.ease_factor = std::max(1.3,
        s.ease_factor + (0.1 - (5 - score) * (0.08 + (5 - score) * 0.02)));
    return s;
}
```

### 4.3 搜索引擎（src/search/）

**两种搜索：**

1. **简单子串搜索** — `bagu search 关键词`，扫数据库 LIKE 查询
2. **FTS5 全文搜索** — SQLite 自带，支持中文分词（用 jieba 或简易 N-gram）
3. **进阶：模糊搜索** — Fuzzy matching（Levenshtein 距离）

**初版实现：**
```cpp
// 用 SQLite FTS5
auto results = db.query(
    "SELECT * FROM card_fts WHERE card_fts MATCH ? ORDER BY rank LIMIT 20",
    keyword);
```

### 4.4 TUI 层（src/tui/）

用 **FTXUI**（C++ 现代终端 UI 库）。

**界面草图：**

```
┌── bagu-cli ─────────────────────────────────────────┐
│                                                      │
│  Topic: MySQL > 第3章 索引                            │
│                                                      │
│  Q: 为什么 InnoDB 选 B+ 树而不是 B 树？                │
│                                                      │
│  ┌─────────────────────────────────────────────┐    │
│  │  [按空格显示答案]                            │    │
│  └─────────────────────────────────────────────┘    │
│                                                      │
│  进度: ███████░░░  7/10 今日已复习                  │
└──────────────────────────────────────────────────────┘
[1] 完全忘记  [2] 印象  [3] 答对  [4] 流畅  [5] 完美  [q] 退出
```

### 4.5 LLM API（src/llm/）

**抽象接口：**

```cpp
class LLMClient {
public:
    virtual ~LLMClient() = default;
    virtual std::string ask(const std::string& prompt) = 0;
    virtual void stream(const std::string& prompt,
                        std::function<void(const std::string&)> on_chunk) = 0;
};

class OpenAIClient : public LLMClient { ... };
class ClaudeClient : public LLMClient { ... };
class OllamaClient : public LLMClient { ... };  // 本地模型
```

**面试模式 prompt 模板：**

```
你是一名资深 C++ 后端面试官。请围绕主题 [{topic}] 提问。
要求：
1. 一次问一个问题
2. 用户回答后，给出 0-10 分评分和改进建议
3. 根据用户表现，下一题难度自适应
当前用户答题历史：{history}
```

### 4.6 配置（~/.bagu/config.toml）

```toml
[general]
data_dir = "~/.bagu"
docs_dir = "~/workspaces/bagu-docs"
default_topic = "mysql"

[review]
daily_target = 20            # 每天复习目标
new_cards_per_day = 5         # 每天新学

[llm]
provider = "openai"           # openai | claude | ollama
api_key_env = "OPENAI_API_KEY"
model = "gpt-4o-mini"
base_url = ""                 # 自定义 endpoint

[ui]
theme = "dark"                # dark | light
hide_progress_in_tui = false
```

---

## 五、命令设计

### 顶层命令

```bash
bagu init                           # 首次初始化（建库、写默认配置）
bagu import <path>                  # 导入 md 文档
bagu list [topic]                   # 列出主题/章节
bagu search <keyword>               # 搜索
bagu review                          # 进入复习 TUI
bagu review --topic mysql           # 仅复习某主题
bagu interview --topic mysql        # AI 模拟面试
bagu stats                           # 统计信息
bagu stats --heatmap                # GitHub 风格热力图
bagu config get/set <key> [value]
bagu version
bagu help
```

---

## 六、目录约定

```
~/.bagu/
├── config.toml         # 用户配置
├── bagu.db             # SQLite 数据库
├── cache/              # LLM 缓存
└── logs/               # 日志
```

---

## 七、开发路线（详见 plan.md）

- **Week 1**：项目骨架 + SQLite + CLI11 + 配置加载 + 简单命令（init / version / config）
- **Week 2**：Markdown 解析器 + 导入 + 列表 + 搜索
- **Week 3**：复习算法（SM-2） + TUI 复习模式
- **Week 4**：LLM 集成 + 面试模式 + 统计 + 热力图

---

## 八、技术选型理由

| 选型           | 替代品              | 理由                                        |
|--------------|--------------------|-------------------------------------------|
| C++20        | C++17               | concept、coroutine（如果用上）、ranges       |
| CMake        | Bazel / Meson       | 通用，求职标配                                |
| FetchContent | vcpkg / conan       | 零依赖（不需要预装包管理器）                    |
| SQLite       | LevelDB / 文件      | 轻量、SQL、内置 FTS5 全文搜索                  |
| CLI11        | argparse            | 现代 C++ 风格、文档全                         |
| FTXUI        | ncurses             | 现代 C++、声明式、易用                        |
| nlohmann/json | rapidjson          | 易用，性能足够                                |
| libcurl      | cpp-httplib         | 工业标准，HTTPS / SSE 都成熟                  |
| cmark        | md4c               | C 库，CommonMark 标准                       |
| spdlog       | glog                | 现代、性能好                                  |
| GoogleTest   | Catch2              | 业界标准                                     |
