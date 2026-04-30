# Web UI 架构（HLD）

> **状态**：Draft
> **版本**：v1.0
> **作者**：Li Yan
> **创建**：2026-04-30
> **上游**：[overview.md](./overview.md)

---

## 1. 设计目标

| 维度 | 目标 |
|----|----|
| 一致性 | Web UI 行为与 CLI 等价（同一份 service 层）|
| 性能 | API < 50ms / 首屏 < 500ms |
| 安全 | 默认 127.0.0.1，可选 token |
| 单二进制 | 前端资源内嵌到 bagu，不依赖外部静态服务 |
| 跨设备 | 桌面 + 平板 + 手机浏览器 |
| 离线优先 | 不依赖任何 CDN / 第三方在线服务 |

---

## 2. 总体架构

```
┌─────────────────────────────────────────────────┐
│        Browser (Desktop / Mobile)                │
│  ┌──────────────────────────────────────────┐    │
│  │  React SPA (Vite build)                   │    │
│  │  ├─ TopicList / ChapterTree / CardView     │    │
│  │  ├─ Search (debounced)                     │    │
│  │  ├─ ReviewSession (SM-2 UI)                │    │
│  │  ├─ InterviewChat (SSE streaming)          │    │
│  │  └─ Stats + Heatmap                        │    │
│  └──────────────────────────────────────────┘    │
└──────────────────┬──────────────────────────────┘
                   │ REST / JSON over HTTP
                   │ SSE for streaming (interview)
                   ▼
┌─────────────────────────────────────────────────┐
│           bagu serve (HTTP server)               │
│  ┌──────────────────────────────────────────┐    │
│  │  cpp-httplib                              │    │
│  │  ├─ Router (/api/*)                       │    │
│  │  ├─ Handlers → 调用现有 service 层        │    │
│  │  ├─ JSON 序列化 (nlohmann/json)           │    │
│  │  ├─ SSE 推送（interview 流式）            │    │
│  │  └─ 静态资源（embed 编译期）              │    │
│  └─────────┬────────────────────────────────┘    │
│            │                                       │
│            ▼                                       │
│  ┌──────────────────────────────────────────┐    │
│  │  service 层（当前已有，零改动）            │    │
│  │  ImportService / ReviewService            │    │
│  │  InterviewService / StatsService          │    │
│  └─────────┬────────────────────────────────┘    │
│            │                                       │
│            ▼                                       │
│  ┌──────────────────────────────────────────┐    │
│  │  db / parser / llm / util                 │    │
│  └──────────────────────────────────────────┘    │
└─────────────────────────────────────────────────┘
                   │
                   ▼
       ~/.bagu/bagu.db  (SQLite WAL)
```

### 2.1 关键约束

- **service 层零改动** — Web UI 只是另一个调用方
- **CLI 和 server 可同时使用同一 db** — SQLite WAL 模式支持
- **server 是无状态的** — 进程崩溃 / 重启不丢业务数据
- **前端资源编译期内嵌** — `bagu serve` 单进程即可，不依赖 `npm run dev` 之类

---

## 3. 模块边界

### 3.1 后端新增

| 模块 | 路径 | 职责 |
|----|----|----|
| http_server | src/http/ | cpp-httplib 启动 + 路由注册 |
| handlers | src/http/handlers/ | 单个 endpoint 的处理函数 |
| json_io | src/http/json_io.h | model ↔ JSON 序列化 |
| sse | src/http/sse.h | SSE 流式响应辅助 |
| static_assets | build/ | 编译期生成的前端资源 .h |
| cli/serve_cmd | src/cli/serve_cmd.cpp | `bagu serve` 命令 |

### 3.2 前端项目结构

```
web/                        ← 前端独立子项目
├── package.json
├── vite.config.ts
├── tsconfig.json
├── tailwind.config.ts      ← 用 Tailwind CSS（轻量、无运行时）
├── index.html
└── src/
    ├── main.tsx
    ├── App.tsx
    ├── api/
    │   ├── client.ts       ← fetch 封装
    │   ├── topics.ts
    │   ├── cards.ts
    │   ├── search.ts
    │   ├── review.ts
    │   ├── interview.ts    ← SSE
    │   └── stats.ts
    ├── pages/
    │   ├── HomePage.tsx
    │   ├── TopicPage.tsx
    │   ├── CardPage.tsx
    │   ├── SearchPage.tsx
    │   ├── ReviewPage.tsx
    │   ├── InterviewPage.tsx
    │   └── StatsPage.tsx
    ├── components/
    │   ├── MarkdownRenderer.tsx
    │   ├── Heatmap.tsx
    │   ├── ReviewCard.tsx
    │   ├── ScoreButtons.tsx
    │   └── ProgressBar.tsx
    ├── hooks/
    │   └── useApi.ts       ← SWR 风格
    └── styles/
        └── tokens.css
```

### 3.3 编译流程

```
1. 前端 build：
   cd web && npm install && npm run build
   产出：web/dist/index.html + assets/*

2. 资源嵌入：
   scripts/embed-assets.sh 把 web/dist/** 转成 build/static_assets.h
   （使用 xxd 或 cmake-bin2c）

3. CMake：
   - 把 static_assets.h 加入 src/http/CMakeLists.txt
   - 编译 bagu 单二进制

4. 运行：
   bagu serve --port 8780
   → 内嵌的前端资源直接由 server 返回
```

---

## 4. 数据流

### 4.1 列出主题

```
Browser:  GET /api/topics
   ↓
Router:   handlers::topics::list
   ↓
Service:  TopicDao::find_all + 各表 count
   ↓
JSON:     [{ "id": 1, "name": "mysql", "cards": 86, ... }]
```

### 4.2 复习评分

```
Browser:  POST /api/review/<card_id>/grade  { "score": 4, "duration_ms": 5000 }
   ↓
Handler:  ReviewService::submit_review(card_id, score, duration_ms)
   ↓
DB:       upsert review + insert review_history（事务）
   ↓
JSON:     { "next_review": 1700000000, "interval_days": 6, "ease": 2.6 }
```

### 4.3 AI 面试（SSE 流）

```
Browser:  POST /api/interview/start  → { "session_id": 42 }
Browser:  GET /api/interview/42/next-question  (SSE)
   ↓
Server:   InterviewService::next_question 流式 → 逐 chunk 写 SSE
   ↓
Browser:  EventSource 接收，逐字渲染

Browser:  POST /api/interview/42/answer  { "question": "...", "answer": "..." }
   ↓
Server:   ReviewService::grade_answer 流式 → SSE
   ↓
Browser:  接收评分文本，并存到 session
```

---

## 5. 关键技术决策

详见 ADR：

- [ADR-0004 add HTTP server mode](./adr/0004-add-http-server-mode.md)
- [ADR-0005 use cpp-httplib for HTTP](./adr/0005-choose-cpp-httplib.md)
- [ADR-0006 frontend stack: Vite + React](./adr/0006-frontend-stack-vite-react.md)

---

## 6. 部署模型

### 6.1 开发期

- 后端：`cmake --build build --target bagu` → `./build/src/bagu serve --port 8780 --dev`
- 前端：`cd web && npm run dev`（vite dev server on :5173，proxy 到 8780）
- 浏览器：`http://localhost:5173`（dev mode）

### 6.2 生产期（Release）

```bash
make release-web
# 触发：
# 1. cd web && npm run build
# 2. scripts/embed-assets.sh
# 3. cmake build with embedded assets

bagu serve  # 单进程，端口 8780
```

### 6.3 跨设备访问

```bash
# 仅本机
bagu serve

# 同 WiFi 局域网（小心：同网络他人也可访问）
bagu serve --bind 0.0.0.0 --token mySecret123

# Tailscale（推荐）：默认 bind 127.0.0.1 + Tailscale 自动暴露
tailscale up
bagu serve --bind 100.x.x.x
```

---

## 7. 质量属性策略

### 7.1 性能

- 首屏：HTML + 内嵌关键 CSS + 异步加载 JS bundle
- API 响应：DAO 层已 < 5ms / 整链路 < 50ms
- 大量列表：服务端分页 + 前端虚拟滚动

### 7.2 可靠性

- API 全部走 service 层（已有覆盖测试）
- 新增 handler 单测（用 cpp-httplib 提供的测试 client）
- 前端用 React Error Boundary 兜底

### 7.3 可观测性

- spdlog 加 access log（method / path / status / duration_ms）
- `--verbose` 时打印请求 body
- 前端 console.error 可选上报到 `/api/_log`（v0.4 加）

### 7.4 安全

详见 [security-checklist.md](../quality/security-checklist.md) 的新增条目。

---

## 8. 演进策略

### 8.1 不破坏 CLI

- Web UI 是新增入口
- 所有现有 CLI 命令照常可用
- 数据库 schema 向后兼容

### 8.2 服务端模式（未来）

如果某天想做云端版本，目前架构允许：
- 把 SQLite 换成 PostgreSQL（改 db 层）
- 把 service 层做成 grpc / RESTful 服务
- 前端基本不动（仅改 API base URL）

---

## 9. 不做的设计选择

| 不做 | 理由 |
|----|----|
| WebSocket | SSE 已经够用，复杂度低 |
| Server-Side Rendering | 个人工具不需要 SEO |
| GraphQL | 接口简单，REST 够 |
| Redux 等状态管理 | React Context + SWR 即可 |
| 自研 UI 框架 | Tailwind CSS + headless UI 现成 |
| 多语言 i18n | v0.3/0.4 不做，统一中文 |

---

## 10. 相关文档

- [PRD-web-ui.md](../product/PRD-web-ui.md)
- [http-api-spec.md](./http-api-spec.md)
- [web-ui-roadmap.md](../planning/web-ui-roadmap.md)
- [security-checklist.md](../quality/security-checklist.md)
