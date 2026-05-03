# bagu-cli

> 八股文档智能学习助手 — 用 C++ 写的命令行工具，帮你高效复习面试八股

[![CI](https://github.com/chunluren/bagu-cli/actions/workflows/ci.yml/badge.svg)](https://github.com/chunluren/bagu-cli/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Tests](https://img.shields.io/badge/tests-224%20passed-brightgreen.svg)](#)
[![Version](https://img.shields.io/badge/version-v1.2.0-brightgreen.svg)](https://github.com/chunluren/bagu-cli/releases/tag/v1.2.0)

> **📖 [用户手册](./docs/user-guide/README.md)** — 安装 / 配置 / Web UI / LLM 接入 / FAQ

---

## 这是什么

把零散的 markdown 八股文档（MySQL / Redis / C++ 网络等）变成一个可交互的复习系统：

- **导入与索引** — 解析 markdown，自动抽取「Q-A 卡片」与「小节卡片」
- **全文搜索** — SQLite FTS5 倒排索引，毫秒级查询，关键词高亮
- **智能复习** — 基于艾宾浩斯遗忘曲线（**SM-2** 算法）推送复习题
- **TUI 交互** — FTXUI 全屏复习界面，按键评分（0-5）
- **AI 模拟面试** — 调用 OpenAI / Claude / Ollama 出题 + 评分（流式 SSE）
- **学习统计** — 连续打卡 / 各主题进度 / 薄弱排行 / Unicode 热力图
- **Web UI** — `bagu serve` 启动单进程 HTTP server，桌面/手机浏览器即可访问（v0.3 新增）
- **本地优先** — 数据全部本地 SQLite，不上传

---

## 快速演示

```bash
# 1. 初始化（创建 ~/.bagu/）
$ bagu init

# 2. 导入你的 markdown 八股文档（自动判重，支持目录递归）
$ bagu import /path/to/bagu-docs/
Importing from /path/to/bagu-docs/...
Found 4 markdown file(s).

  [1/4] cpp-network-interview.md  → cpp-network  (new, 165 cards)
  [2/4] mysql-interview.md        → mysql        (new, 86 cards)
  [3/4] redis-interview.md        → redis        (new, 144 cards)
  [4/4] README.md                  → readme       (new, 0 cards)

Done in 0.16s. Total cards: 395

# 3. 看看导入了什么
$ bagu list
  TOPIC                  CARDS    CHAPTERS  TITLE
  ----------------------------------------------------------------
  cpp-network              165         102  C++ 网络编程八股文档（高级版）
  mysql                     86          78  MySQL 八股文档（高级版）
  redis                    144          79  Redis 八股文档（高级版）
  ----------------------------------------------------------------
  Total: 4 topics, 395 cards

# 4. 看某个主题的章节树
$ bagu list mysql

  MySQL 八股文档（高级版）
  86 cards · 78 chapters

  ├── 第 1 章 MySQL 架构与基础  (7 cards)
  │   ├── 1.1 整体架构
  │   ├── 1.2 一条 SELECT 的执行流程
  │   └── ...
  ├── 第 3 章 索引（重点）  (13 cards)
  │   ├── 3.1 为什么选 B+ 树？
  │   └── ...
  └── ...

# 5. 模糊搜索（关键词高亮，含章节定位）
$ bagu search MVCC

Found 5 cards for "MVCC" (0ms)

  [#194] mysql / 第 4 章 事务与 MVCC
      MVCC 原理（核心）
      ...多版本并发控制，让读写不互相阻塞...

  [#196] mysql / 第 4 章 事务与 MVCC
      Read View 完整可见性算法
      ...每个版本 V（其 trx_id = v_trx）...

  ...

# 6. 看单张卡片完整内容
$ bagu show 194
========== Card #194 ==========
Topic:    mysql
Chapter:  第 4 章 事务与 MVCC（重点）
Type:     section

Q: MVCC 原理（核心）

A: ...

# 7. 进入交互式复习（FTXUI 全屏）
$ bagu review --topic mysql -n 10
  ╭──────────────────────────────────────────────────────────╮
  │ bagu review              Card 1/10 · mysql · NEW          │
  │                                                            │
  │ ━━━━━━━━━━━━━━━━━━━━━━                                    │
  │                                                            │
  │ Q:                                                         │
  │ MVCC 原理（核心）                                          │
  │                                                            │
  │ ───────────────────────────────                           │
  │   按 SPACE / Enter 显示答案                                │
  │                                                            │
  ╰──────────────────────────────────────────────────────────╯
   [SPACE] 显示答案  [s] 跳过  [q] 退出

# 8. AI 模拟面试（流式输出，OpenAI/Claude/Ollama）
$ export OPENAI_API_KEY=sk-...
$ bagu interview --topic mysql -n 3

========== AI 模拟面试 ==========
主题: mysql · 题数: 3 · 提供商: openai · 模型: gpt-4o-mini

─── 第 1/3 题 ───

[面试官] 请简要说明 MVCC 是如何实现非阻塞读的？
                  涉及哪些数据结构？
[你] (多行答案，单独一行 . 或 Ctrl+D 结束；q 退出会话)
> MVCC 通过隐藏字段和 undo log 版本链...
> .

[评分] 评分：8/10
       ✓ 答对：隐藏字段、undo 版本链、Read View 三要素
       ✗ 缺漏：未说明 RC 与 RR 下 Read View 生成时机的差异
       💡 建议：补充快照读 vs 当前读对比

# 9. 启动 Web UI（v0.3 新增，单进程含前端）
$ bagu serve
[2026-05-03 21:40] Embedded Web UI: 5 assets
[2026-05-03 21:40] bagu serve listening on http://127.0.0.1:8780
# 浏览器打开 http://localhost:8780
# - 主题列表 / 章节树 / 卡片详情（marked + 代码高亮）
# - 即时搜索（debounced + 关键词高亮）
# - 复习页（键盘 0-5 评分 + 移动端触屏 + 进度条 + 完成总结）

# 局域网手机访问（家庭 WiFi）：
$ bagu serve --bind 0.0.0.0 --token mySecret
# 手机浏览器：http://<电脑 IP>:8780
# 请求需带：Authorization: Bearer mySecret

# 10. 学习统计（含 unicode 热力图）
$ bagu stats --heatmap

========== 总览 ==========
  连续打卡:   7 天
  累计复习:   42 次
  整体正确率: 76.2%
  已学卡片:   28 / 395

========== 各主题进度 ==========
  TOPIC                LEARNED  ACCURACY    DUE
  ------------------------------------------------
  mysql                  18/86      78%      6
  redis                   8/144     62%      2
  cpp-network             2/165     50%      -

========== 最薄弱（最近答错最多）==========
  1. [mysql]  MVCC 原理（核心）        答错 3/4 次
  2. [redis]  Set 和 ZSet 区别？        答错 2/3 次

========== 热力图 (最近 90 天) ==========
  Mon   ··▁▃▅▇▇▅▃▁······▁▃▅▇▇▇█▇▅▃▁
  Tue   ▁▃▅▇▅▃▁······▁▃▇█▇▅▃▁······
  Wed   ····▁▃▅▇▅▃▁······▁▃▅▇▇▅▃▁··
  ...
  Less ·▁ ▃▅ ▇█ More
```

按 SPACE 看答案 → 1-5 评分 → 自动下一题 → 退出后看复习总结。

---

## 安装

### Homebrew（推荐 · macOS arm64 / Linux x86_64）

```bash
brew tap chunluren/bagu
brew install bagu-cli
```

### 下载预编译二进制

```bash
# Linux x86_64
curl -LO https://github.com/chunluren/bagu-cli/releases/latest/download/bagu-v0.4.0-linux-x86_64.tar.gz
tar -xzf bagu-v0.4.0-linux-x86_64.tar.gz && sudo mv bagu /usr/local/bin/

# macOS Apple Silicon
curl -LO https://github.com/chunluren/bagu-cli/releases/latest/download/bagu-v0.4.0-macos-arm64.tar.gz
tar -xzf bagu-v0.4.0-macos-arm64.tar.gz && sudo mv bagu /usr/local/bin/
```

### 从源码构建

需要：CMake 3.20+ / GCC 11+ 或 Clang 14+ / Node 20+ / SQLite3 / libcurl / OpenSSL（开发库）。

```bash
# Ubuntu / Debian
sudo apt install -y build-essential cmake ninja-build \
    libsqlite3-dev libcurl4-openssl-dev libssl-dev nodejs npm

# macOS
brew install cmake ninja sqlite curl openssl node

git clone https://github.com/chunluren/bagu-cli.git
cd bagu-cli

# 1. 构建前端（产物 web/dist 自动嵌入二进制）
cd web && npm ci && npm run build && cd ..

# 2. 构建 C++（首次会下 CLI11 / spdlog / FTXUI / cpp-httplib，~1-3 分钟）
cmake -G Ninja -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

sudo cp build/src/bagu /usr/local/bin/
```

---

## 命令参考

| 命令 | 说明 |
|----|----|
| `bagu init [--force]` | 初始化 `~/.bagu/`，写默认配置 |
| `bagu import <path> [--force]` | 导入 markdown 文件或目录（递归）|
| `bagu list [topic]` | 列出主题汇总 / 单主题章节树 |
| `bagu show <id>` | 查看单张卡片完整内容 |
| `bagu search <keyword> [--topic T] [-n N]` | FTS5 全文搜索 |
| `bagu review [--topic T] [-n N] [--new-only] [--all]` | 进入复习 TUI |
| `bagu interview --topic T [-n N] [--provider P] [--model M]` | AI 模拟面试 |
| `bagu stats [--heatmap] [--days N]` | 学习统计 + 热力图 |
| `bagu serve [--port 8780] [--bind] [--token]` | 启动 Web UI（单进程含前端）|
| `bagu config get <key>` / `set <key> <value>` / `list` | 配置管理 |
| `bagu --version` / `--help` | 版本与帮助 |

---

## 数据存储

```
~/.bagu/
├── config.toml      # 用户配置
├── bagu.db          # SQLite 数据库（含 FTS5 索引）
└── logs/            # 日志（spdlog 滚动）
```

**数据全部本地 + SQLite 单文件**，备份只需 `cp bagu.db`。

通过 `BAGU_HOME` 环境变量可改根目录（用于隔离环境 / 测试）。

---

## 文档

完整文档体系（30+ 份）见 [docs/README.md](./docs/README.md)。

### 用户向
- [产品需求 PRD](./docs/product/PRD.md)
- [CLI 命令规范](./docs/architecture/cli-spec.md)

### 开发向
- [架构总览（HLD）](./docs/architecture/overview.md)
- [模块详细设计（LLD）](./docs/architecture/modules.md)
- [数据模型（含 SQL DDL）](./docs/architecture/data-model.md)
- [架构决策（ADR）](./docs/architecture/adr/README.md)
- [C++ 编码规范](./docs/development/coding-standards.md)
- [Git 工作流](./docs/development/git-workflow.md)
- [开发环境搭建](./docs/development/setup.md)
- [测试策略](./docs/testing/strategy.md)

### 运维向
- [构建说明](./docs/operations/build.md)
- [CI/CD](./docs/operations/ci-cd.md)
- [发布流程](./docs/operations/release.md)
- [日志规范](./docs/operations/logging.md)
- [错误处理与错误码](./docs/operations/error-handling.md)

### 规划向
- [Roadmap](./docs/planning/roadmap.md)
- [Milestones](./docs/planning/milestones.md)
- [风险登记册](./docs/planning/risks.md)

### 质量
- [性能预算](./docs/quality/performance-budget.md)
- [安全清单](./docs/quality/security-checklist.md)

---

## 技术栈

| 类别 | 选择 | 用途 |
|----|----|----|
| 语言 | C++20 | 核心 |
| 构建 | CMake 3.20+ + FetchContent | 零依赖（首次拉源码自动编译）|
| 存储 | SQLite3 + FTS5 | 数据库 + 全文搜索 |
| CLI | CLI11 | 命令行解析 |
| TUI | FTXUI v5 | 终端界面 |
| 配置 | toml++ | TOML 解析 |
| HTTP | libcurl | （未来 LLM 接入）|
| 加密 | OpenSSL | SHA256 文件指纹 |
| 日志 | spdlog | 结构化日志 |
| JSON | nlohmann/json | （未来 LLM）|
| 测试 | GoogleTest | **115 个单测，100% 通过** |

---

## 性能（v0.1 实测）

| 操作 | 实测 | 目标 |
|----|----|----|
| `bagu --version` 启动 | < 30ms | < 50ms ✓ |
| 导入 4 文件 / 395 cards | 0.16s | 1000 cards/s ✓ |
| FTS 全文搜索 | < 1ms | < 100ms ✓ |
| TUI 启动到首屏 | < 200ms | < 500ms ✓ |

---

## 路线图

- ✅ **v0.1.0**（2026-04） — MVP：init/import/list/search/show/review + SM-2 + FTXUI
- ✅ **v0.2.0**（2026-04） — `bagu interview` AI 模拟面试 + `bagu stats` 学习统计 + 热力图
- ✅ **v0.3.0**（2026-05，当前）— `bagu serve` Web UI（单进程含 React 前端）
- ⏳ **v0.4.0** — Web UI AI 面试页（SSE 流式）+ 统计页 + 热力图 + PWA
- ⏳ **v1.0.0** — 完整 CI matrix、Homebrew formula、Debian package

详见 [Roadmap](./docs/planning/roadmap.md)。

---

## 项目结构

```
bagu-cli/
├── CMakeLists.txt
├── README.md / CHANGELOG.md / CONTRIBUTING.md / LICENSE
├── docs/                    ← 30+ 份文档
├── src/
│   ├── main.cpp
│   ├── cli/                 ← 子命令分发
│   │   ├── init_cmd.cpp / config_cmd.cpp
│   │   ├── import_cmd.cpp / list_cmd.cpp / show_cmd.cpp
│   │   ├── search_cmd.cpp / review_cmd.cpp
│   ├── service/             ← 应用服务
│   │   ├── import_service.cpp / review_service.cpp
│   ├── core/                ← 领域逻辑
│   │   └── sm2.cpp
│   ├── db/                  ← SQLite 封装
│   │   ├── database.cpp / statement.cpp / migrations.cpp
│   │   ├── topic_dao.cpp / chapter_dao.cpp
│   │   ├── card_dao.cpp / review_dao.cpp
│   ├── parser/              ← Markdown 解析
│   │   └── markdown_parser.cpp
│   ├── tui/                 ← FTXUI 界面
│   │   └── review_screen.cpp
│   └── util/
│       ├── path.cpp / hash.cpp
├── include/bagu/            ← 公共头文件
│   ├── error.h / result.h / version.h
└── tests/                   ← 90 个单测
    └── unit/
        ├── core/ db/ parser/ service/ util/
```

---

## 贡献

欢迎贡献！请先读 [CONTRIBUTING.md](./CONTRIBUTING.md)。

```bash
# 跑测试
cd build && ctest --output-on-failure

# 提交规范（Conventional Commits）
git commit -m "feat(cli): add 'bagu xxx' command"
```

---

## License

[MIT](./LICENSE)

---

## 致谢

- [Anki](https://apps.ankiweb.net/) — 间隔重复算法的灵感
- [muduo](https://github.com/chenshuo/muduo) — 现代 C++ 服务端实践
- 各位作者贡献的开源依赖（CLI11 / spdlog / FTXUI / SQLite / ...）
