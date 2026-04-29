# Changelog

本项目所有显著变更都将记录在此文件中。

格式遵循 [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)，
版本号遵循 [Semantic Versioning](https://semver.org/spec/v2.0.0.html)。

---

## [Unreleased]

（暂无变更）

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

[Unreleased]: https://github.com/chunluren/bagu-cli/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/chunluren/bagu-cli/releases/tag/v0.1.0
