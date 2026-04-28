# 数据模型

> **状态**：Approved
> **版本**：v1.0
> **更新**：2026-04-29

---

## 1. ER 图

```
┌────────┐       ┌─────────┐       ┌──────────────┐
│ topic  │──1:N──│ chapter │──1:N──│ card          │
└────────┘       └─────────┘       └──────────────┘
                                          │ 1:1
                                          ▼
                                   ┌──────────────┐
                                   │  review       │
                                   └──────────────┘
                                          │ 1:N
                                          ▼
                                   ┌──────────────┐
                                   │review_history │
                                   └──────────────┘

┌──────────────────┐       ┌──────────────┐
│interview_session │──1:N──│interview_qa  │
└──────────────────┘       └──────────────┘
```

---

## 2. SQLite Schema

### 2.1 完整 DDL

```sql
-- ========== 主题（Topic）==========
CREATE TABLE topic (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    name        TEXT NOT NULL UNIQUE,        -- 'mysql' / 'redis'
    title       TEXT NOT NULL,                -- 'MySQL 八股文档'
    file_path   TEXT NOT NULL,                -- 源 md 文件绝对路径
    file_hash   TEXT NOT NULL,                -- 内容 SHA256，判重导入
    imported_at INTEGER NOT NULL,             -- unix timestamp
    updated_at  INTEGER NOT NULL
);

CREATE INDEX idx_topic_name ON topic(name);

-- ========== 章节（Chapter）==========
CREATE TABLE chapter (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    topic_id    INTEGER NOT NULL,
    name        TEXT NOT NULL,                -- '索引'
    chapter_no  INTEGER NOT NULL,             -- 3
    parent_id   INTEGER,                      -- 父章节（支持多级），NULL=顶级
    FOREIGN KEY (topic_id) REFERENCES topic(id) ON DELETE CASCADE,
    FOREIGN KEY (parent_id) REFERENCES chapter(id) ON DELETE CASCADE,
    UNIQUE (topic_id, chapter_no, parent_id)
);

CREATE INDEX idx_chapter_topic ON chapter(topic_id);

-- ========== 卡片（Card）==========
CREATE TABLE card (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    topic_id        INTEGER NOT NULL,
    chapter_id      INTEGER,
    question        TEXT NOT NULL,
    answer          TEXT NOT NULL,
    difficulty      INTEGER DEFAULT 2,        -- 1=易 / 2=中 / 3=难
    tags            TEXT,                     -- JSON array, e.g. ["mysql","index"]
    source_line     INTEGER,                  -- md 文件中的行号
    card_type       TEXT DEFAULT 'qa',        -- 'qa' / 'concept' / 'code'
    created_at      INTEGER NOT NULL,
    updated_at      INTEGER NOT NULL,
    FOREIGN KEY (topic_id) REFERENCES topic(id) ON DELETE CASCADE,
    FOREIGN KEY (chapter_id) REFERENCES chapter(id) ON DELETE SET NULL
);

CREATE INDEX idx_card_topic ON card(topic_id);
CREATE INDEX idx_card_chapter ON card(chapter_id);
CREATE INDEX idx_card_difficulty ON card(difficulty);

-- ========== 复习状态（Review，SM-2）==========
CREATE TABLE review (
    card_id         INTEGER PRIMARY KEY,
    last_review     INTEGER,                  -- 上次复习时间戳，NULL=未学
    next_review     INTEGER,                  -- 下次复习时间戳
    interval_days   INTEGER DEFAULT 0,        -- 当前间隔（天）
    ease_factor     REAL DEFAULT 2.5,         -- SM-2 难度因子
    repetitions     INTEGER DEFAULT 0,        -- 连续答对次数
    review_count    INTEGER DEFAULT 0,        -- 总复习次数
    correct_count   INTEGER DEFAULT 0,        -- 答对次数（评分 >= 3）
    suspended       INTEGER DEFAULT 0,        -- 0/1 暂停
    FOREIGN KEY (card_id) REFERENCES card(id) ON DELETE CASCADE
);

CREATE INDEX idx_review_next ON review(next_review) WHERE suspended = 0;

-- ========== 复习历史 ==========
CREATE TABLE review_history (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    card_id     INTEGER NOT NULL,
    reviewed_at INTEGER NOT NULL,
    score       INTEGER NOT NULL,             -- 0-5
    duration_ms INTEGER,                      -- 答题耗时
    FOREIGN KEY (card_id) REFERENCES card(id) ON DELETE CASCADE
);

CREATE INDEX idx_history_card ON review_history(card_id);
CREATE INDEX idx_history_time ON review_history(reviewed_at);

-- ========== 全文搜索（FTS5）==========
CREATE VIRTUAL TABLE card_fts USING fts5(
    question,
    answer,
    tags,
    content='card',
    content_rowid='id',
    tokenize='unicode61 remove_diacritics 1'
);

-- 同步 trigger
CREATE TRIGGER card_fts_insert AFTER INSERT ON card BEGIN
    INSERT INTO card_fts(rowid, question, answer, tags)
    VALUES (new.id, new.question, new.answer, COALESCE(new.tags, ''));
END;

CREATE TRIGGER card_fts_delete AFTER DELETE ON card BEGIN
    DELETE FROM card_fts WHERE rowid = old.id;
END;

CREATE TRIGGER card_fts_update AFTER UPDATE ON card BEGIN
    UPDATE card_fts
    SET question = new.question,
        answer = new.answer,
        tags = COALESCE(new.tags, '')
    WHERE rowid = new.id;
END;

-- ========== 面试会话 ==========
CREATE TABLE interview_session (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    topic           TEXT NOT NULL,
    started_at      INTEGER NOT NULL,
    ended_at        INTEGER,
    total_score     REAL,                     -- 0-10
    question_count  INTEGER DEFAULT 0,
    llm_provider    TEXT,
    llm_model       TEXT
);

CREATE TABLE interview_qa (
    id              INTEGER PRIMARY KEY AUTOINCREMENT,
    session_id      INTEGER NOT NULL,
    question_no     INTEGER NOT NULL,
    question        TEXT NOT NULL,
    user_answer     TEXT,
    ai_score        INTEGER,                  -- 0-10
    ai_feedback     TEXT,
    duration_ms     INTEGER,
    FOREIGN KEY (session_id) REFERENCES interview_session(id) ON DELETE CASCADE
);

CREATE INDEX idx_qa_session ON interview_qa(session_id);

-- ========== Schema 版本（用于迁移）==========
CREATE TABLE schema_version (
    version     INTEGER PRIMARY KEY,
    applied_at  INTEGER NOT NULL,
    description TEXT
);

-- 初始版本
INSERT INTO schema_version (version, applied_at, description)
VALUES (1, strftime('%s', 'now'), 'initial schema');
```

### 2.2 索引策略

| 索引                        | 用途                            | 频次   |
|---------------------------|-------------------------------|------|
| idx_topic_name            | 按主题名查找                       | 高    |
| idx_chapter_topic         | 列出某主题的章节                    | 高    |
| idx_card_topic            | 按主题取卡片                       | 高    |
| idx_review_next（部分索引）  | 查到期复习的卡片                    | 极高  |
| idx_history_time          | 时间范围查询（热力图）                | 中    |

---

## 3. 实体说明

### 3.1 Topic
- 一个 markdown 文档文件 = 一个 topic
- `name` 全小写英文，作为命令参数（`bagu list mysql`）
- `title` 是显示名（用户可读）
- `file_hash` 用于检测内容变化，决定是否重新解析

### 3.2 Chapter
- 对应 markdown 的 `## 第 N 章 XXX` 与 `### N.M XXX`
- 通过 `parent_id` 支持多级（章 → 节）

### 3.3 Card
- 复习的最小单元
- 来源两种：
  - **Q/A 抽取**：`**Q1. 问题** 答案`（题库章节）
  - **小节抽取**：整个 `### X.Y` 三级标题作为 question，正文作为 answer
- `tags` 用 JSON 数组方便扩展
- `card_type`：未来支持代码题、概念题等

### 3.4 Review（SM-2 状态）
- 一张卡片对应一条 review 记录
- 新卡片首次显示前 review 行可能不存在（lazy 创建）
- `next_review` 是关键查询字段

### 3.5 Review History
- 每次评分都追加一条
- 用于：热力图、答错率分析、回滚

### 3.6 Interview Session / QA
- 一次面试 = 一个 session
- 每个问题 + 答案 + AI 评分 = 一条 QA

---

## 4. 数据生命周期

```
导入 md
  ↓
  抽取 topic / chapter / card  →  入 SQLite
  触发 fts trigger              →  自动建索引

首次复习
  ↓
  card 第一次出现，无 review 记录
  评分后创建 review 行 + history 行

后续复习
  ↓
  按 next_review <= now() 选 due cards
  评分后更新 review，追加 history

删除文档（bagu remove --topic mysql）
  ↓
  ON DELETE CASCADE 自动清理 chapter / card / review / history
```

---

## 5. 数据迁移

每个 schema 变更对应 `src/db/migrations/NNNN_description.sql`：

```
src/db/migrations/
├── 0001_initial.sql
├── 0002_add_card_type.sql
├── 0003_add_interview_tables.sql
└── ...
```

启动时检查 `schema_version`，按顺序执行未应用的 migration。

```cpp
void Database::migrate() {
    int current = get_current_version();
    for (int v = current + 1; v <= LATEST_VERSION; ++v) {
        execute_migration_file(v);
        record_version(v);
    }
}
```

---

## 6. 数据量估算

| 实体             | 量级（单用户）         |
|----------------|-------------------|
| topic          | 5-20             |
| chapter        | 50-200           |
| card           | 500-5000         |
| review         | 与 card 同数量      |
| review_history | 50000+（多年累积）  |
| 数据库总大小      | < 100 MB         |

**结论：** SQLite 完全够用，不需要分库分表。

---

## 7. 备份与恢复

- 数据库就是单文件 `~/.bagu/bagu.db`，**直接 cp 即备份**
- 恢复：把备份文件覆盖即可
- 未来支持：`bagu backup` / `bagu restore` 命令

---

## 8. 性能注意

- **写操作必须用事务**，单条 INSERT 是慢的（fsync 一次）
- 批量导入用 `BEGIN; INSERT...; INSERT...; COMMIT;` 一次提交
- 全表统计查询加 LIMIT
- FTS5 查询用 `MATCH` 而非 `LIKE`

---

## 9. 相关文档

- [架构总览](./overview.md)
- [模块详细设计](./modules.md)
- [ADR-0002 选 SQLite + FTS5](./adr/0002-use-sqlite-with-fts5.md)
