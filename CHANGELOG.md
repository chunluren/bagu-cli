# Changelog

本项目所有显著变更都将记录在此文件中。

格式遵循 [Keep a Changelog](https://keepachangelog.com/en/1.1.0/)，
版本号遵循 [Semantic Versioning](https://semver.org/spec/v2.0.0.html)。

---

## [Unreleased]

### Added
- 项目骨架（CMake / 目录结构 / git 初始化）
- 完整文档体系（PRD / 架构 / ADR / 编码规范 / 测试 / CI / 发布等）
- 入口程序（main.cpp）含所有子命令骨架（暂未实现具体逻辑）
- ADR-0001：选用 C++20
- ADR-0002：选 SQLite + FTS5
- ADR-0003：用 CMake FetchContent 管理依赖

---

## [0.1.0] - TBD (planned 2026-05-19)

### Added
- `bagu init` — 初始化数据目录与配置
- `bagu import <path>` — 导入 markdown 八股文档
- `bagu list` — 列出主题与章节
- `bagu search <keyword>` — 全文搜索
- `bagu review` — 交互式复习（基于 SM-2）
- `bagu config get/set` — 配置管理
- `bagu show <id>` — 查看单卡片
- SQLite + FTS5 数据存储
- TUI 复习界面（FTXUI）
- 中文 N-gram 分词

---

## [0.2.0] - TBD (planned 2026-05-26)

### Added
- `bagu interview` — AI 模拟面试（OpenAI/Claude/Ollama）
- `bagu stats` — 学习统计
- `bagu stats --heatmap` — GitHub 风格热力图
- LLM 流式响应

---

## [1.0.0] - TBD (planned 2026-06-30)

### Added
- 完整 e2e 测试套件
- 多平台二进制发布（Linux / macOS）
- Homebrew formula
- Debian package
- 完整用户文档

---

[Unreleased]: https://github.com/<user>/bagu-cli/compare/v0.1.0...HEAD
[0.1.0]: https://github.com/<user>/bagu-cli/releases/tag/v0.1.0
