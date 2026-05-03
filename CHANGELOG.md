# Changelog

本项目所有显著变更都将记录在此文件中。

格式遵循 [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)，
版本号遵循 [Semantic Versioning](https://semver.org/spec/v2.0.0.html)。

---

## [Unreleased]

### Added — CSV 导出 + 卡片 pause/unpause（v1.2.1）

#### CSV 导出
- `bagu export csv [--topic T] [-o file]` — RFC 4180 格式，Notion / Obsidian / Excel 通吃
- `GET /api/export/csv?topic=&include_section=` — 同结构 + Content-Disposition + X-Bagu-* 头
- 字段含 `,` `"` `\n` `\r` 时用双引号包，内部 `"` 转义为 `""`；行尾 `\r\n`
- 头一行：`id,topic,chapter,question,answer,tags,card_type`
- 3 个新单测覆盖 全量 / 转义（comma/quote/newline）/ topic 过滤

#### 卡片 pause / unpause
- `bagu pause <card_id>` 或 `bagu pause --topic <T>` — 单卡或全主题暂停复习
- `bagu unpause` — 对称恢复
- `POST /api/review/:id/suspend` body `{"suspended":true|false}` — Web UI 用
- `ReviewDao::set_suspended(card_id, bool)` 自动 INSERT OR IGNORE 一行 review（处理新卡场景）
- `ReviewDao::set_suspended_by_topic(topic_id, bool)` 批量 + 返回受影响数
- 4 个新单测：单卡新建 / 已存在保留其他字段 / 主题批量 / toggle 往返

### Added — UX 小优化

- **`bagu serve` 启动打印 LAN IP**：当 `--bind 0.0.0.0` 时枚举本机 IPv4，
  打印「可访问 URL」列表，方便手机直接复制（自动跳过 lo/docker/veth/Clash fakeIP 198.18.x.x）。
  顺手把 `serve` 的默认日志级别从 warn 升到 info，让启动 banner / 嵌入资源数 / access log 都可见
- **`SearchPage` 初始空态**：访问 `/search` 还没输入时显示「N 主题 M 卡片」
  + 10 个常见关键词标签（MVCC / epoll / B+ 树 ...），点一下就触发搜索
- 新增 `src/util/network.{h,cpp}` — `local_ipv4_addresses()` 跨 Linux/macOS

199 单测 + 25 e2e 全过；嵌入资源 12 个不变。

---

## [1.2.0] - 2026-05-03

**今日复习推送版**：让用户每天「打开就知道该做什么」、「关上电脑也会被提醒」。

### Highlights
- 📅 **「今日到期」speed view** — `bagu due` CLI / `GET /api/review/due-summary` / Web 首页 hero banner
- 🔔 **`bagu remind`** — 桌面通知（Linux notify-send / macOS osascript）+ 完整 cron/systemd/launchd 接入文档
- 🐛 **Fix**：SQL LEFT JOIN 把 0 卡 topic 误算成 1 张新卡 → 加 `c.id IS NOT NULL`
- 🐛 **Fix**：e2e stats weak-cards 在 strict-mode 下 flaky → 用更具体的文案

测试：199 C++ 单测（v1.1 +4）+ 25 e2e（v1.1 +1）= 224 总。

完全向后兼容 v1.1.x。

### Added — `bagu remind` 桌面通知（v1.2）

#### 新命令
- `bagu remind [--topic T] [--threshold N] [--quiet] [--dry-run]`
- 阈值检测：`total_due + total_new < threshold` → 不发，return 0
- `--quiet` 不输出 stdout/stderr（适合 cron 不发邮件）
- `--dry-run` 仅打印通知文案

#### 跨平台
- `src/util/notify.{h,cpp}` 抽象桌面通知：
  - Linux：`notify-send --app-name=bagu --icon=accessories-text-editor`
  - macOS：`osascript -e 'display notification ...'`
  - Windows：暂未实现，返回 `kNotImplemented`
- shell 字符串安全转义（single-quote escape）
- `desktop_notify_available()` 用 `command -v` 检测

#### 文档
- 新增 `docs/user-guide/remind.md`：cron / systemd user timer / launchd 三套接入方案 + 退出码 + FAQ

### Added — 「今日到期」速览（v1.2 起步）

#### 后端
- `service::ReviewService::due_summary()` — 各主题的到期数 + 新卡数 + 全局合计
- 单条 SQL：`topic LEFT JOIN card LEFT JOIN review` 按 topic 聚合
  - 修了一个易踩的 LEFT JOIN 坑：空 topic 会被算成 1 张新卡（NULL 行），加 `c.id IS NOT NULL` 过滤
  - 跳过 0 卡片的 topic（如 readme.md）
- 4 个新单测：空 DB / 复习后 due 计数 / 跳过空 topic / 尊重 suspended 标记

#### CLI
- 新命令 `bagu due [--topic T]`：彩色输出今日到期 + 各主题分布 + 下一步提示
- TTY 检测（非交互/重定向时关闭 ANSI 颜色）

#### HTTP
- `GET /api/review/due-summary` — 同前端结构

#### Web
- 首页顶部加「今日到期 N 张」hero banner（点击跳 `/review`）
- 0 到期时显示「🎉 今日没有到期卡片」+ AI 面试 CTA
- 显示前 3 个主题的题目分布速览
- e2e 加 due-summary 接口形状校验

#### 修复 e2e flaky
- `stats.spec.ts` 的 weak-cards 空态用更具体的文案（"没有薄弱卡片，太棒了"）避免与 section heading 冲突

199 单测 + 25 e2e 全通过。

---

## [1.1.0] - 2026-05-03

**用户体验扩展版**：4 个独立特性，零破坏性变更，完全向后兼容 v1.0.x。

### Highlights
- 🃏 **Anki 导出**：`bagu export anki [--topic T] [-o file]` + `GET /api/export/anki`
- 🔑 **稳定 card.id**：重导入 / `--force` 不再丢 SM-2 复习历史（schema v3 → v4，自动 backfill）
- 📜 **Web 面试历史**：`/interview/history` + `/interview/sessions/:id` 两页
- 🎛 **多 LLM profile**：`[llm.profiles.<name>]` 段，`bagu interview --profile cheap`
- 195 单测（v1.0 171 → +24）+ 24 e2e（v1.0 21 → +3）

### Added — LLM 多 profile 配置

#### config.toml 新语法
```toml
[llm]                              # 默认段
provider = "openai"
model = "gpt-4o-mini"
api_key_env = "OPENAI_API_KEY"

[llm.profiles.cheap]               # 用本地 ollama
provider = "ollama"
model = "qwen2.5"

[llm.profiles.serious]             # 部分覆盖：只换 model，继承其他
model = "gpt-4o"
```

#### 接口
- CLI：`bagu interview --profile cheap --topic mysql`
- CLI：`bagu config list-profiles` 列出可用 profile
- HTTP：`POST /api/interview/sessions` body 接受 `profile` 字段
- HTTP：`GET /api/llm/profiles` 返回 `{"profiles":["default","cheap",...]}`

#### 优先级
CLI `--provider`/`--model` > profile 段 > [llm] 默认段。
profile 段没写的字段从默认段继承。

#### 实现
- `ConfigOverride` 加 `profile` 字段
- `load_config` 重写：default → profile → CLI 三级合并；`api_key_env` 也按 profile-优先解析
- `list_profiles()` 新函数
- `docs/user-guide/llm-providers.md` 加完整 profile 章节

#### 测试
- 13 个新单测：默认段加载 / profile 全/部分覆盖 / 优先级 / 不存在 profile / api_key_env 各种路径 / list_profiles
- 共 195 单测，100% 通过

### Added — Web 面试历史页（v1.1）

#### 前端
- 新页 `/interview/history` — 表格列出最近 50 次面试会话（topic / 时间 / 题数 / 平均分 / 用时 / 模型）
- 新页 `/interview/sessions/:id` — 单次会话详情，按题号展开 question / user_answer / ai_feedback + 颜色高亮 score
- `InterviewPage` 设置页底部加「查看历史会话」入口
- 使用已有 `/api/interview/sessions` + `/api/interview/sessions/:id` 接口（v0.4 Sprint 6 时已实现）

#### 测试
- `web/e2e/interview-history.spec.ts` 3 个 e2e：空态 CTA / 跳转链接可见 / 不存在的 session id 不白屏
- 共 24 个 e2e + 182 单测，100% 通过
- 嵌入资源 12 个不变；前端 bundle JS 42KB → 50KB（+8KB 含两个新页）

### Fixed — 重导入保留 SM-2 复习历史（v1.1）

#### 背景
之前 `bagu import --force` 或文件内容变化时会 `DELETE FROM card WHERE topic_id = ?` 全删重建，
ON DELETE CASCADE 顺手把 `review` 和 `review_history` 也清了 — 修个错别字 = 丢全部艾宾浩斯进度。

#### 修复
- Schema migration v4：`card` 加 `stable_key TEXT UNIQUE` 列
- `stable_key = sha256(topic_name + "::" + normalized_question)[:32]`
  - 题面归一化：trim + 内部空白合并为单空格 + ASCII 小写
- `import_service` 改为 upsert：
  - 旧 DB 升级首次 re-import：先按当前 question 文本回填 `stable_key`
  - 然后 `card_dao.upsert_by_stable_key` — 匹配 → UPDATE（保留 id + review 历史），不匹配 → INSERT
  - 最后删除本次没出现的 stale 卡（用临时表传 keep_keys）
- 同一文档内题面归一化后冲突的 → 跳过后面的 + warning

#### 测试
- `ReImport_SameQuestion_PreservesIdAndReviewHistory` — 改答案 + 加新卡，旧卡 id + review + history 全保留
- `ReImport_DroppedQuestion_RemovesCardAndHistory` — 题被删则 CASCADE 清理
- `LegacyCards_NullStableKey_Backfilled_HistoryPreserved` — v3→v4 升级路径
- `NormalizeQuestion_WhitespaceInsensitive` — 空白 / TAB 不影响匹配
- `FirstImport_AssignsStableKey` — 新卡都有 32 字符 stable_key
- 共 182 单测，100% 通过；e2e schema_version 断言同步更新到 4

### Added — Anki 导出（v1.1 起步）

#### 新接口
- CLI：`bagu export anki [--topic T] [-o file] [--include-section]`
- HTTP：`GET /api/export/anki?topic=&include_section=` 含 `Content-Disposition` + `X-Bagu-Export-Total/Written` 头

#### 实现
- `src/service/export_service.{h,cpp}` — 流式写 Anki "Notes in Plain Text"
- 字段处理：TAB → 4 空格、`\n` / `\r\n` → `<br>`
- Anki 标头：`#separator:tab` / `#html:true` / `#columns:Front\tBack\tTags` / `#tags column:3`
- Tag 自动加 `bagu <topic>`，原 tags 字段的逗号 / TAB 转空格
- 默认仅导出 qa 卡，`--include-section` 才带章节占位卡

#### 测试 & 文档
- 6 个单测：全量 / 按 topic / topic 不存在 / TAB+换行转义 / tags 处理
- 共 177 单测，100% 通过
- `docs/user-guide/export.md` — 导出 + Anki 三步导入 + 设计取舍

---

## [1.0.0] - 2026-05-03

**首个稳定版**：到此为止 bagu-cli 已具备生产级品质 — Web UI 完善、跨平台分发、e2e 覆盖、用户手册完整。

### Highlights

- 🍺 **Homebrew 安装**：`brew tap chunluren/bagu && brew install bagu-cli`
- 🌐 **跨平台二进制**：macOS Apple Silicon + Linux x86_64（自动嵌入 Web UI）
- ✅ **192 自动化测试**：171 C++ 单测 + 21 Playwright e2e
- 🚀 **CI matrix 5 组合**：ubuntu-22/24 × gcc/clang + macos-14
- 📖 **完整用户手册**：6 篇 docs/user-guide/
- 🔄 **零数据 serve**：`bagu serve` 在空数据库下也能跑（自动初始化）

### 与 v0.4.0 的差异

v1.0.0 = v0.4.0 + Homebrew tap 发布 + Playwright e2e + 文档完善。
代码层面无破坏性变更；二进制行为与 v0.4.0 一致。

### Added — Homebrew tap

- 仓库 `chunluren/bagu-cli` 由 private 改 public
- 新仓库 `chunluren/homebrew-bagu` — Homebrew tap
- `Formula/bagu-cli.rb` 优先用预编译二进制（macOS arm64 / Linux x86_64），其他平台 fallback 到 `--HEAD` 源码构建
- 安装：`brew tap chunluren/bagu && brew install bagu-cli`
- README + 用户手册首次推荐 brew 安装路径

### Added — Playwright e2e（v0.4 Sprint 9）

#### `web/`
- 引入 `@playwright/test` + chromium-headless-shell
- `playwright.config.ts` 自动启动 `bagu serve`，配 `BAGU_HOME=/tmp/bagu-e2e` 隔离数据
- `e2e/*.spec.ts` 21 个用例：
  - `home.spec.ts` — 首页 nav / 5 入口 / 主题切换可见 / `/api/health`
  - `theme.spec.ts` — 三态主题切换 / localStorage 持久化 / 刷新后保留
  - `navigation.spec.ts` — 5 路由渲染 + SPA 404 fallback
  - `stats.spec.ts` — 4 张统计卡 / 热力图 days 切换 / 薄弱卡片空态
  - `api-smoke.spec.ts` — stats / interview 入参 / PWA 资源 mime
- `e2e/README.md` 跑测说明 + 故障排查
- 配 `--no-proxy-server` + `--proxy-bypass-list=<-loopback>` 避免 Clash / shadowsocks 拦截 localhost

#### CI
- `.github/workflows/ci.yml` 新增 `e2e` job（ubuntu-24，并行 build-test）：
  - 构建 binary + npm ci + chromium install + 跑 21 用例
  - 失败时上传 `playwright-report` artifact

#### 仓库
- `.gitignore` 加 `web/test-results/` `web/playwright-report/` `web/playwright/.cache/`

---

## [0.4.0] - 2026-05-03

**Web UI 完善版**：把 v0.3 的 SPA 骨架填满，并补全工程基础。

### Highlights
- 🤖 **AI 模拟面试** Web 页（流式 SSE + 多轮历史折叠 + 完成总结）
- 📊 **学习统计** Web 页（GitHub 风格热力图 + 主题进度 + 薄弱卡片）
- 🌓 **主题三态切换**（亮 / 暗 / 跟随系统，无首屏闪白）
- 📱 **PWA**（manifest + service worker + 桌面/手机一键安装）
- 🛡 **代码评审遗留批量修复**（SQLite FULLMUTEX / migration 拆事务 / curl OOM / OpenAI 协议层抽出）
- 🚀 **CI matrix 扩到 5 组合** + release.yml 接入 web build（修复 v0.3 binaries 漏 UI 的 bug）
- 📖 **6 份用户手册**

171 单测全通过 · 嵌入资源 7 → 12 · 单二进制 ~41 MB（含完整前端）。

---

### Docs — 用户手册

新增 `docs/user-guide/`：
- `getting-started.md` — 5 分钟上手（安装 / init / import / 复习 / 面试 / serve）
- `config.md` — `~/.bagu/config.toml` 全字段
- `web-ui.md` — `bagu serve` 路由 / API / 局域网手机访问 / PWA 安装
- `llm-providers.md` — OpenAI / Claude / Ollama / 兼容代理 / 错误码
- `markdown-format.md` — 题库 markdown 格式规范 + 常见坑
- `faq.md` — 安装 / 数据 / 复习 / 面试 / 性能 / 隐私 / 调试

`docs/README.md` 与 README.md 顶部均加入用户手册入口。

### CI/CD — 多平台 matrix + Web 构建接入（v1.0 准备）

#### `.github/workflows/ci.yml`
- 新增 `web-build` job：vite + tsc 构建后上传 `web-dist` artifact
- `build-test` matrix 增至 5 组合：ubuntu-22 (gcc/clang) + ubuntu-24 (gcc/clang) + macos-14 (arm64)
- 各平台从 artifact 下载 `web/dist`，二进制内嵌真正的前端
- 新增 HTTP server smoke 子步骤：启动 `bagu serve`，curl 7 个关键端点 + PWA 资源
- 新增 `web-lint` job：`tsc -b --noEmit` 类型检查
- 移除 macos-13（Intel）从 CI（仅在 release 构建）

#### `.github/workflows/release.yml`
- **(critical)** 新增 `web-build` 前置 job — 之前发布的二进制根本没有 Web UI
- 新增 `Verify embedded assets` 步骤：grep `manifest+json` 字符串，确保前端真的进了 binary
- 释出包：linux-x86_64 / macos-x86_64 / macos-arm64

### Fixed — 代码评审遗留批量修复

- **(important)** SQLite 连接 `OPEN_NOMUTEX → OPEN_FULLMUTEX`。`bagu serve` 多线程 handler
  共用一个 Database 实例，原来的 NOMUTEX 是真实 UB；并发请求 smoke 测试通过
- **(important)** `db::Database::migrate` 拆每个 migration 独立事务。
  失败时不丢已成功的版本，下次启动只重试失败那一版
- **(important)** `util/http.cpp` `make_header_list` 修 `curl_slist_append` OOM 泄漏：
  失败时释放已累积 list、向调用方返回 `kNetworkError`，避免静默丢 header 与内存泄漏
- **(refactor)** `OpenAIClient` 协议层抽出纯函数 namespace `openai_protocol`：
  `build_chat_payload` / `parse_chat_response` / `parse_sse_line` / `map_http_status` /
  `normalize_base_url`。无 IO，独立单测

### Added

- 21 个 OpenAI 协议层单测（payload 构建 / 响应解析 / SSE 解析 / 状态码映射 / URL 规范）
- 共 171 单测，100% 通过

### Added — 主题切换 + PWA（v0.4 Sprint 8）

#### 主题手动切换
- Tailwind 切换 `darkMode: 'media' → 'class'`
- `web/src/hooks/useTheme.ts` — light / dark / system 三态，持久化到 localStorage；system 模式实时跟随 prefers-color-scheme
- `web/src/components/ThemeToggle.tsx` — 顶部 3 段开关（图标按钮）
- `bootstrapTheme()` 在 React 挂载前同步 `.dark` class，无首屏闪白
- highlight.js 主题改为 class-toggleable（手写 .dark .hljs-* 样式）

#### PWA
- `public/manifest.webmanifest` — display=standalone + 3 个快捷入口（复习 / 面试 / 统计）
- `public/sw.js` — 极简 SW：app shell cache-first + /api/* network-only + 导航 fallback /
- `public/icon-{192,512}.png` + `apple-touch-icon.png`（Pillow 生成 · 八字 logo）
- `public/favicon.svg` 替换为 bagu-themed 简洁 SVG
- `index.html` 增 manifest / apple-touch-icon / theme-color (light+dark) / apple-mobile-web-app-* 元
- `pwa.ts` 在生产构建里 `navigator.serviceWorker.register('/sw.js')`，dev 模式跳过避免 HMR 冲突
- `gen_embedded_assets.cmake` 新增 `.webmanifest` mime 映射
- 嵌入资源数 7 → 12（含 PNG 二进制）

### Added — Web 学习统计页（v0.4 Sprint 7）

#### Backend
- 4 个新接口（`HttpServer::register_stats_routes`）：
  - `GET /api/stats/overall` — 总览（streak / 累计复习 / 正确率 / 30 天活跃）
  - `GET /api/stats/topics` — 各主题进度（已学/总数 + 正确率 + 今日到期）
  - `GET /api/stats/heatmap?days=N` — 每日复习计数（夹紧到 1-365）
  - `GET /api/stats/weak?recent=N&top=K` — 最近答错最多的卡片

#### Frontend
- `web/src/components/Heatmap.tsx` — GitHub 风格热力图（按周排列、5 级颜色、点击跳转复习页）
- `web/src/pages/StatsPage.tsx` — 完整统计页：4 张总览卡 + 热力图（30/90/180/365 切换）+ 主题进度条 + 薄弱卡片排行
- 移除 PlaceholderPage（不再需要）

#### Tests
- 6 个新增 HTTP 单测（overall / topics / heatmap exact-days / heatmap clamp / weak empty / weak after failures）
- 共 150 单测，100% 通过

### Added — Web 模拟面试（v0.4 Sprint 6）

#### Backend
- `src/llm/config_loader.{h,cpp}` — 从 CLI 提取，REST 与 CLI 共享 LLM 配置加载逻辑
- `src/http/interview_routes.{h,cpp}` — 5 个新接口：
  - `POST /api/interview/sessions` — 创建会话（topic + question_count + 可选 provider/model 覆盖）
  - `GET  /api/interview/sessions` — 最近会话列表
  - `GET  /api/interview/sessions/:id` — 会话详情 + qa list
  - `GET  /api/interview/sessions/:id/question` — SSE 流式出题
  - `POST /api/interview/sessions/:id/answer` — SSE 流式评分
  - `POST /api/interview/sessions/:id/finish` — 结束会话写入 ended_at + avg_score
- SSE 帧格式：`data: {"type":"chunk","text":"…"}` / `{"type":"done",…}` / `{"type":"error",…}`
- 进程内 `ServiceRegistry` 缓存进行中会话的 `InterviewService`（保留 system prompt）

#### Frontend
- `web/src/api/client.ts` 新增 `streamSSE()` — 用 fetch 而非 EventSource（支持 POST + body）
- `web/src/pages/InterviewPage.tsx` — 完整 UI：
  - 设置表单（主题 / 题数 / 高级 LLM 覆盖）
  - 流式出题打字机效果
  - 答题输入（Cmd/Ctrl+Enter 提交）
  - 流式评分 + 颜色高亮分数
  - 多轮历史折叠展示
  - 完成总结页
  - 错误恢复（一键返回设置）
- 顶部导航新增「面试」入口

#### Tests
- 9 个新增 HTTP 单测（CreateSession 入参校验 / ListSessions / GetSession / Answer 与 Finish 在 session 不在内存时的 404）
- 共 144 单测，100% 通过

---

## [0.3.0] - 2026-05-03

**Web UI MVP**：把 CLI 工具变成跨设备应用。

### Added

#### `bagu serve` — 嵌入式 HTTP server
- 单进程提供 REST API + 前端 SPA（资源编译期内嵌到二进制）
- 选项：`--bind` / `--port` / `--token` / `--dev`
- 默认 `127.0.0.1:8780`（仅本机），可选 `--bind 0.0.0.0` + `--token` 局域网访问
- SIGINT/SIGTERM 优雅退出
- 基于 cpp-httplib v0.18.5

#### REST API
- `GET /api/health` / `GET /api/version`
- `GET /api/topics` / `GET /api/topics/:name`
- `GET /api/cards/:id` / `GET /api/cards?topic=`
- `GET /api/search?q=&topic=&limit=`
- `GET /api/review/due?topic=&max_due=&max_new=`
- `POST /api/review/:id/grade { score, duration_ms }`
- 标准错误响应：`{ "error": { "code", "message", "detail" } }`
- 错误码映射 HTTP 状态：4020/4021 → 404, 2xxx → 400, 5xxx → 502

#### Web 前端（React 18 + TypeScript + Tailwind 3 + Vite）
- 4 页：HomePage / TopicPage / CardPage / SearchPage / ReviewPage
- `<MarkdownRenderer>` — marked + highlight.js（仅注册 14 种常见语言）
- `<ScoreButtons>` — 0-5 评分按钮 + 键盘快捷键
- `<ReviewPage>` — 完整 SM-2 复习会话
  - 进度条 / NEW 标记 / 已答对统计
  - SPACE/Enter 揭示答案
  - 评分后显示 'next in N days · ease X.XX'
  - 完成总结页
- 暗色模式跟随系统
- 移动端友好（响应式 + 触屏大按钮）
- 总 bundle：380 KB（gzip 约 120 KB，达成 PRD < 300 KB 目标）

#### 资源嵌入
- `scripts/gen_embedded_assets.cmake` — 把 `web/dist` 转 C++ unsigned char[]
- `src/http/embedded_assets.h` + 自动生成的 `.cpp`
- HttpServer 404 fallback：精确匹配 → SPA 路由（fallback `/index.html`）
- 单进程 `bagu serve` 即可，**无需任何静态文件部署**

### 文档
- `docs/product/PRD-web-ui.md` — Web UI 需求
- `docs/architecture/web-ui-overview.md` — 架构设计
- `docs/architecture/http-api-spec.md` — API 规范
- `docs/architecture/adr/0004-add-http-server-mode.md`
- `docs/architecture/adr/0005-choose-cpp-httplib.md`
- `docs/architecture/adr/0006-frontend-stack-vite-react.md`
- `docs/planning/web-ui-roadmap.md` — 9 sprint 实施计划

### 测试
- 16 个新增 HTTP server 单测（含 review 5 个）
- 共 135 个单测，100% 通过

### Performance
- 单二进制：39 MB（含 React/marked/highlight.js）
- 启动 < 100ms
- API 响应 < 5ms（同 CLI 性能）
- bundle 首屏 < 500ms

### Stack
基础（v0.2 已有）+ cpp-httplib v0.18.5 + React 18 + Tailwind 3 + Vite + marked + highlight.js + lucide-react + react-router-dom

---

## [0.2.1] - 2026-04-30

代码评审反馈修复版。功能未变。

### Fixed

- **(critical)** `ImportService` 重新导入时拼字符串删数据，已改为 prepared statement + bind
  （破坏 DAO 层不变量，未来字段类型变更会引入注入风险）
- **(critical)** `MarkdownParser` 的 `std::stoi` 不捕异常，超长数字（>INT_MAX）会一路冒泡崩溃，已改用 `std::from_chars`
- **(important)** `interview_cmd.cpp` 缺 `<unistd.h>` 但使用 `::isatty()`，依赖传递性 include
- **(important)** FTS5 `MATCH` 直接接受 keyword，含 OR/AND/NEAR/`*`/`-` 等关键字会报 malformed expression
  改为用 phrase quote（双引号包围 + 内部 `"` 转义）作字面量
- **(important)** `stats_service.cpp:local_midnight` `mktime` 未设 `tm_isdst = -1`，跨夏令时切换日会偏移 1 小时
- **(important)** `interview_cmd.cpp` toml 二次 `**get_as<>()` 解引用 + 无 nullptr 校验，已改为单次取值 + 显式校验
- **(important)** Prompt injection 防御：`grading_prompt` 用 `<user_answer>` XML 标签包围用户输入，
  system prompt 添加"标签内容仅作评分对象、不改变评分立场"的硬约束

### Added

- 4 个回归测试覆盖：FTS5 关键字 escape / FTS5 双引号 / 章号 INT_MAX 越界 / 子章号越界
- 共 119 单测，100% 通过

---

## [0.2.0] - 2026-04-30

### Added

#### AI 模拟面试
- `bagu interview --topic <T> [-n N] [--provider P] [--model M]`
- LLM 抽象层（OpenAI / Claude / Ollama / OpenAI 兼容服务）
- 流式 SSE 输出（题目和评分实时显示）
- 配置加载：`~/.bagu/config.toml` + 环境变量 API key
- 评分自动解析：支持 `评分：N/10` / `Score: N` / `分数：N` 等格式
- 多行答案输入（`.` 或 `Ctrl+D` 结束，`q` 退出会话）
- 会话历史完整保存到 `interview_session` + `interview_qa` 表
- Schema migration v3：interview 相关表

#### 学习统计
- `bagu stats` — 总览（连续打卡、累计复习、整体正确率、各主题进度、薄弱卡片）
- `bagu stats --heatmap [--days N]` — Unicode 块字符热力图（按周排版）
- 总览：连续打卡天数、最近 30 天活跃日数、整体正确率
- 各主题进度：已学/总数、正确率、今日到期数
- 薄弱排行：最近答错最多的卡片（默认 top 5）

#### 基础设施
- libcurl HTTP 工具（同步 + 流式 SSE）
- 错误码细分：网络 / HTTP / LLM 认证 / 限流 / 无效响应
- LLM 工厂模式（按 provider 路由）

### Tests
- 26 个新增测试（共 115 个，100% 通过）
- LLM Mock + Factory: 9 个
- InterviewDao + InterviewService: 10 个
- StatsService: 6 个

---

## [0.1.0] - 2026-04-30

**首个 MVP 版本** — 完整可用的本地八股文档复习工具。

### Added

#### 核心命令
- `bagu init [--force]` — 初始化 `~/.bagu/` 数据目录与默认配置
- `bagu import <path> [--force]` — 导入 markdown 文件 / 目录（递归扫描，SHA256 判重）
- `bagu list [topic]` — 列出主题汇总 / 单主题章节树
- `bagu show <id>` — 显示单张卡片完整内容（topic / chapter / Q / A）
- `bagu search <keyword> [--topic T] [-n N]` — FTS5 全文搜索（关键词高亮）
- `bagu review [--topic T] [-n N] [--new-only] [--all]` — 进入 FTXUI 复习 TUI
- `bagu config get / set / list` — 配置管理（TOML）
- `bagu --version / --help` — CLI11 标准

#### 基础设施
- 错误体系：`Result<T>` + `Error` + 错误码体系（E1xxx-E9xxx）+ 退出码映射
- 路径处理：`util::path` 支持 `~/` 展开 + `BAGU_HOME` 环境变量
- SHA256：`util::hash` 基于 OpenSSL EVP API
- SQLite 封装：`Database` / `Statement` / `Transaction` (RAII)
- Schema migration：版本化 SQL，自动应用未执行的迁移

#### 数据层
- v1 schema：topic / chapter / card / review / review_history
- v2 schema：FTS5 虚拟表 + INSERT/UPDATE/DELETE 触发器自动同步
- DAO：`TopicDao` / `ChapterDao` / `CardDao` / `ReviewDao`

#### 解析层
- 手写 markdown 解析器（不依赖 cmark）
  - `# 标题` → ParsedDocument.title
  - `## 第 N 章` → 顶级 Chapter
  - `### N.M` → 子 Chapter + section card
  - `**Q数字. ?** 答` → qa card（编号格式）
  - `**Q: ?** 答` → qa card（冒号格式）
  - 代码块（` ``` `）内不抽取
- 文件名 → topic 名推断（去除 `-interview` / `八股文档` 后缀）

#### 服务层
- `ImportService`：扫描 → SHA256 判重 → 解析 → 事务批量入库
- `ReviewService`：取卡 → SM-2 计算 → upsert + 历史

#### 算法
- SM-2 算法（SuperMemo 2）
  - 0-2 失败：repetitions 重置，interval=1
  - 3-5 成功：interval = 1/6/prev*ease
  - ease factor 调整 + 下限 1.3

#### TUI
- FTXUI v5 集成
- 复习界面：进度条 + Q + 隐藏答案 + 0-5 评分
- 快捷键：SPACE 揭示 / 1-5 评分 / s 跳过 / q 退出 / ? 帮助

#### 文档
- 30+ 份生产级文档（PRD / 架构 / ADR / 编码规范 / 测试 / CI / 发布 / 性能 / 安全 / Roadmap / Risks / DoD）
- ADR-0001 选用 C++20
- ADR-0002 选 SQLite + FTS5
- ADR-0003 用 CMake FetchContent

#### 测试
- 90 个单元测试，100% 通过
- 覆盖：core (SM-2) / db (database/statement/migrations/各 DAO) / parser / service (import/review) / util / Result

### Performance

| 操作 | 实测 | SLO |
|---|---|---|
| `bagu --version` 启动 | < 30ms | < 50ms ✓ |
| 导入 4 文件 / 395 cards | 0.16s | 1000 cards/s ✓ |
| FTS 搜索 | < 1ms | < 100ms ✓ |
| TUI 启动到首屏 | < 200ms | < 500ms ✓ |

### Tech Stack

C++20 · CMake · SQLite3 + FTS5 · CLI11 · FTXUI · toml++ · libcurl · OpenSSL · spdlog · nlohmann/json · GoogleTest

---

## [0.2.0] - 计划 2026-05-26

### 计划 Added
- `bagu interview` — AI 模拟面试（OpenAI / Claude / Ollama）
- `bagu stats` — 学习统计
- `bagu stats --heatmap` — GitHub 风格热力图
- LLM 流式响应（SSE）
- 中文 N-gram 分词（提升 FTS 中文精度）

---

## [1.0.0] - 计划 2026-06-30

### 计划 Added
- 完整 e2e 测试套件
- 多平台二进制发布（Linux / macOS）
- Homebrew formula
- Debian package
- 完整用户文档

---

[Unreleased]: https://github.com/chunluren/bagu-cli/compare/v1.2.0...HEAD
[1.2.0]: https://github.com/chunluren/bagu-cli/compare/v1.1.0...v1.2.0
[1.1.0]: https://github.com/chunluren/bagu-cli/compare/v1.0.0...v1.1.0
[1.0.0]: https://github.com/chunluren/bagu-cli/compare/v0.4.0...v1.0.0
[0.4.0]: https://github.com/chunluren/bagu-cli/compare/v0.3.0...v0.4.0
[0.3.0]: https://github.com/chunluren/bagu-cli/compare/v0.2.1...v0.3.0
[0.2.1]: https://github.com/chunluren/bagu-cli/compare/v0.2.0...v0.2.1
[0.2.0]: https://github.com/chunluren/bagu-cli/compare/v0.1.0...v0.2.0
[0.1.0]: https://github.com/chunluren/bagu-cli/releases/tag/v0.1.0
