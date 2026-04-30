# HTTP API 规范

> **状态**：Draft
> **版本**：v1.0
> **更新**：2026-04-30

---

## 1. 通用约定

### 1.1 Base URL

```
默认：http://127.0.0.1:8780
```

### 1.2 内容类型

- 请求体：`application/json; charset=utf-8`
- 响应体：`application/json; charset=utf-8`
- SSE 流：`text/event-stream; charset=utf-8`

### 1.3 鉴权（可选）

- 默认不要求（仅本机）
- `bagu serve --token <SECRET>` 启用 Bearer 鉴权
  ```
  Authorization: Bearer <SECRET>
  ```
- 缺失或错误返回 401

### 1.4 通用错误响应

```json
{
  "error": {
    "code": 4020,
    "message": "卡片不存在",
    "detail": "id=99999 not found"
  }
}
```

错误码与 CLI 一致（详见 [error-handling.md](../operations/error-handling.md)）。

### 1.5 HTTP 状态码映射

| HTTP | 含义 | 错误码段 |
|----|----|----|
| 200 | 成功 | - |
| 201 | 已创建（POST） | - |
| 204 | 无内容 | - |
| 400 | 参数错误 | 2xxx |
| 401 | 未鉴权 | - |
| 404 | 资源不存在 | 4020 / 4021 |
| 409 | 冲突 | 4012 |
| 500 | 服务器内部错误 | 9xxx |
| 502 | LLM 上游失败 | 5xxx |

### 1.6 分页约定

```
GET /api/cards?page=1&size=20&topic=mysql
```

响应：
```json
{
  "items": [...],
  "page": 1,
  "size": 20,
  "total": 86,
  "has_more": true
}
```

---

## 2. Endpoints

### 2.1 元信息

#### `GET /api/health`

健康检查。

**响应：**
```json
{ "ok": true, "version": "0.3.0", "schema_version": 3 }
```

#### `GET /api/version`

```json
{ "version": "0.3.0", "build_date": "Apr 30 2026" }
```

---

### 2.2 主题（Topics）

#### `GET /api/topics`

列出所有主题。

**查询参数：** 无

**响应 200：**
```json
[
  {
    "id": 1,
    "name": "mysql",
    "title": "MySQL 八股文档（高级版）",
    "cards": 86,
    "chapters": 78,
    "learned": 18,
    "due_today": 5,
    "imported_at": 1714000000
  }
]
```

#### `GET /api/topics/:name`

主题详情，包含章节树。

**路径参数：** `name`（如 `mysql`）

**响应 200：**
```json
{
  "id": 1,
  "name": "mysql",
  "title": "MySQL 八股文档（高级版）",
  "chapters": [
    {
      "id": 10,
      "chapter_no": 1,
      "name": "MySQL 架构与基础",
      "card_count": 7,
      "subchapters": [
        { "id": 11, "chapter_no": 1001, "name": "1.1 整体架构", "card_count": 0 }
      ]
    }
  ]
}
```

**响应 404：** topic not found

---

### 2.3 卡片（Cards）

#### `GET /api/cards/:id`

获取单张卡片完整内容。

**响应 200：**
```json
{
  "id": 194,
  "topic_id": 1,
  "topic_name": "mysql",
  "chapter_id": 50,
  "chapter_label": "第 4 章 事务与 MVCC",
  "question": "MVCC 原理（核心）",
  "answer": "**Multi-Version Concurrency Control** ...",
  "difficulty": 2,
  "tags": [],
  "card_type": "section",
  "source_line": 355,
  "review": {
    "last_review": 1700000000,
    "next_review": 1700500000,
    "interval_days": 6,
    "ease_factor": 2.6,
    "review_count": 3,
    "correct_count": 2
  }
}
```

`review` 字段：从未学过则为 `null`。

#### `GET /api/cards`

列出卡片（分页）。

**查询：** `?topic=mysql&chapter=10&page=1&size=20`

**响应：** 标准分页结构

---

### 2.4 搜索（Search）

#### `GET /api/search?q=<keyword>`

全文搜索。

**查询：**
- `q` (必填)：关键词
- `topic`：限定主题
- `limit`：上限（默认 20）

**响应 200：**
```json
{
  "query": "MVCC",
  "took_ms": 8,
  "items": [
    {
      "card_id": 194,
      "topic": "mysql",
      "chapter_label": "第 4 章 事务与 MVCC",
      "question": "MVCC 原理（核心）",
      "snippet": "...多版本并发控制，让读写不互相阻塞...",
      "highlights": [{ "start": 12, "len": 4 }]
    }
  ]
}
```

---

### 2.5 复习（Review）

#### `GET /api/review/due`

获取今日待复习的卡片。

**查询：**
- `topic`：限定主题（可选）
- `max_due`：到期上限（默认 20）
- `max_new`：新卡上限（默认 5）

**响应 200：**
```json
{
  "cards": [
    {
      "card_id": 100,
      "topic": "mysql",
      "question": "Q?",
      "answer": "A...",
      "card_type": "qa",
      "is_new": false,
      "review": { "interval_days": 1, "ease_factor": 2.5, "repetitions": 0 }
    }
  ]
}
```

#### `POST /api/review/:card_id/grade`

提交评分。

**请求体：**
```json
{ "score": 4, "duration_ms": 5000 }
```

**响应 200：**
```json
{
  "card_id": 100,
  "next_review": 1700604800,
  "interval_days": 6,
  "ease_factor": 2.6,
  "repetitions": 1,
  "review_count": 3,
  "correct_count": 2
}
```

**约束：** score ∈ [0, 5]；不在范围返回 400。

---

### 2.6 AI 面试（Interview）

#### `POST /api/interview/start`

创建会话。

**请求体：**
```json
{
  "topic": "mysql",
  "question_count": 5,
  "provider": "openai",
  "model": "gpt-4o-mini"
}
```

**响应 201：**
```json
{ "session_id": 42, "started_at": 1700000000 }
```

#### `GET /api/interview/:id/next-question` (SSE)

流式取下一题。

**事件流：**
```
event: chunk
data: {"text": "请简要"}

event: chunk
data: {"text": "说明 MVCC..."}

event: done
data: {"question_no": 1, "full_question": "..."}
```

#### `POST /api/interview/:id/answer`

提交答案 + 流式接收评分。

**请求体：**
```json
{
  "question_no": 1,
  "question": "请简要说明 MVCC...",
  "answer": "MVCC 是..."
}
```

**响应：** SSE 流，事件类型同上。

`done` 事件：
```json
{
  "score": 7,
  "feedback": "评分：7/10\n✓ ...\n✗ ...\n💡 ..."
}
```

#### `POST /api/interview/:id/end`

结束会话。

**请求体：** 无

**响应 200：**
```json
{
  "session_id": 42,
  "total_questions": 5,
  "answered": 5,
  "avg_score": 7.4,
  "ended_at": 1700001000
}
```

#### `GET /api/interview/:id`

查看会话历史。

**响应 200：**
```json
{
  "session": { "id": 42, "topic": "mysql", "avg_score": 7.4, ... },
  "qa_list": [
    { "question_no": 1, "question": "...", "user_answer": "...", "ai_score": 7, "ai_feedback": "..." }
  ]
}
```

---

### 2.7 统计（Stats）

#### `GET /api/stats/overall`

```json
{
  "total_topics": 4,
  "total_cards": 395,
  "learned_unique_cards": 28,
  "total_reviews": 42,
  "total_correct": 32,
  "overall_accuracy": 0.762,
  "streak_days": 7,
  "active_days_30": 12
}
```

#### `GET /api/stats/topics`

数组，每项同 `/api/topics`，但更详细：包含 accuracy / due_today 等。

#### `GET /api/stats/heatmap?days=90`

```json
{
  "days": 90,
  "data": [
    { "date": "2026-02-01", "count": 0 },
    { "date": "2026-02-02", "count": 5 },
    ...
  ]
}
```

#### `GET /api/stats/weak?recent=50&top=10`

```json
{
  "items": [
    { "card_id": 100, "topic": "mysql", "question": "MVCC?", "wrong_count": 3, "total_recent": 4 }
  ]
}
```

---

### 2.8 导入（Import，可选）

#### `POST /api/import`

触发导入（需要服务端能访问指定路径）。

**请求体：**
```json
{ "path": "/home/user/bagu-docs/", "force": false }
```

**响应 200：** 同 CLI `bagu import` 的 summary。

> ⚠️ v0.3 暂不开放给 Web UI（仅 CLI 使用），避免误用。

---

### 2.9 配置（Config）

#### `GET /api/config`

返回当前 config.toml 内容（脱敏 api_key_env）。

#### `PUT /api/config`

更新配置（merge 模式）。

```json
{ "review.daily_target": 30 }
```

---

## 3. SSE 详细规范

### 3.1 事件类型

| event | data 内容 |
|----|----|
| `chunk` | `{ "text": "片段..." }` |
| `done` | 业务完成数据（见各 endpoint） |
| `error` | `{ "code": 5012, "message": "..." }` |

### 3.2 心跳

每 15 秒发送注释行 `: keepalive\n\n` 防止代理超时。

### 3.3 客户端使用

```ts
const es = new EventSource('/api/interview/42/next-question');
es.addEventListener('chunk', (e) => {
  const { text } = JSON.parse(e.data);
  appendToUI(text);
});
es.addEventListener('done', (e) => {
  const result = JSON.parse(e.data);
  finalize(result);
  es.close();
});
es.addEventListener('error', (e) => {
  showError(JSON.parse(e.data));
  es.close();
});
```

---

## 4. 限流（暂不实现）

v0.3 因为是单机使用，无限流。
v1.0 如果 `--bind 0.0.0.0` 公开访问，加令牌桶限流（10 req/s/IP）。

---

## 5. CORS

默认仅允许同源。开发模式（`--dev`）下允许 `http://localhost:5173`（Vite dev server）。

---

## 6. OpenAPI

v0.3 不维护正式 OpenAPI YAML（手写文档同步）。
v1.0 评估接入 [openapi-cpp](https://github.com/...) 自动生成。

---

## 7. 相关文档

- [web-ui-overview.md](./web-ui-overview.md)
- [error-handling.md](../operations/error-handling.md)
- [security-checklist.md](../quality/security-checklist.md)
