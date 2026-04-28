# bagu-cli

> 八股文档智能学习助手 — 用 C++ 写的命令行工具，帮你高效复习面试八股

[![CI](https://img.shields.io/badge/CI-pending-yellow.svg)](#)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](./LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue.svg)](https://en.cppreference.com/w/cpp/20)
[![Status](https://img.shields.io/badge/status-WIP-orange.svg)](#)

---

## 这是什么

把零散的 markdown 八股文档（MySQL / Redis / C++ 网络等）变成一个可交互的复习系统：

- **智能复习** — 基于艾宾浩斯遗忘曲线（SM-2）推送复习题
- **全文搜索** — 模糊匹配快速找到任何知识点
- **交互式 TUI** — 终端里看题、答题、打分
- **AI 模拟面试** — 调 LLM API 让 AI 提问并自动评分
- **进度统计** — 哪些章节看过、薄弱在哪、刷题热力图
- **本地优先** — 数据全部本地存储，不上传

---

## 状态

🚧 **开发中（v0.1 进行）** — 详见 [Roadmap](./docs/planning/roadmap.md) 和 [Milestones](./docs/planning/milestones.md)

---

## 快速开始

> 注意：v0.1 尚未发布，下面是预期用法。

```bash
# 编译
git clone https://github.com/<user>/bagu-cli.git
cd bagu-cli
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Release ..
ninja

# 初始化
./src/bagu init

# 导入八股文档
./src/bagu import /path/to/bagu-docs/

# 列出所有章节
./src/bagu list

# 进入交互复习
./src/bagu review

# 模糊搜索
./src/bagu search "MVCC 可见性"

# AI 模拟面试
./src/bagu interview --topic mysql --num 5

# 学习统计
./src/bagu stats --heatmap
```

---

## 文档导航

完整文档见 [docs/README.md](./docs/README.md)。

### 用户向
- [产品需求](./docs/product/PRD.md)
- [CLI 命令规范](./docs/architecture/cli-spec.md)

### 开发向
- [开发环境](./docs/development/setup.md)
- [编码规范](./docs/development/coding-standards.md)
- [Git 工作流](./docs/development/git-workflow.md)
- [架构总览](./docs/architecture/overview.md)
- [模块详细设计](./docs/architecture/modules.md)
- [数据模型](./docs/architecture/data-model.md)
- [测试策略](./docs/testing/strategy.md)
- [架构决策（ADR）](./docs/architecture/adr/README.md)

### 运维向
- [构建说明](./docs/operations/build.md)
- [CI/CD](./docs/operations/ci-cd.md)
- [发布流程](./docs/operations/release.md)
- [日志规范](./docs/operations/logging.md)
- [错误处理](./docs/operations/error-handling.md)

### 规划向
- [Roadmap](./docs/planning/roadmap.md)
- [Milestones](./docs/planning/milestones.md)
- [风险登记册](./docs/planning/risks.md)
- [Definition of Done](./docs/planning/definition-of-done.md)

### 质量
- [性能预算](./docs/quality/performance-budget.md)
- [安全清单](./docs/quality/security-checklist.md)

---

## 技术栈

| 类别       | 选择                                       |
|----------|------------------------------------------|
| 语言       | C++20                                    |
| 构建       | CMake 3.20+ + FetchContent               |
| 存储       | SQLite3 + FTS5                           |
| CLI 解析   | CLI11                                    |
| TUI       | FTXUI                                    |
| HTTP      | libcurl                                  |
| Markdown  | cmark                                    |
| 配置       | toml++                                   |
| JSON      | nlohmann/json                            |
| 日志       | spdlog                                   |
| 测试       | GoogleTest + Google Benchmark            |

---

## 项目结构

```
bagu-cli/
├── CMakeLists.txt
├── README.md                  ← 你在这
├── CHANGELOG.md
├── CONTRIBUTING.md
├── LICENSE
├── .gitignore
├── docs/                      ← 完整文档
│   ├── README.md              ← 文档索引
│   ├── product/                ← 产品文档
│   ├── architecture/           ← 架构 + ADR
│   ├── development/            ← 开发规范
│   ├── testing/                ← 测试策略
│   ├── operations/             ← 运维文档
│   ├── quality/                ← 质量保障
│   ├── planning/               ← 项目规划
│   └── reference/              ← 参考资料
├── src/                       ← 源码
│   ├── main.cpp
│   ├── cli/        # 命令分发
│   ├── service/    # 应用层服务
│   ├── core/       # 领域逻辑（SM-2 等）
│   ├── db/         # SQLite 封装
│   ├── parser/     # Markdown 解析
│   ├── search/     # 全文搜索
│   ├── llm/        # LLM API 客户端
│   ├── tui/        # FTXUI 界面
│   └── util/       # 通用工具
├── include/bagu/              ← 公共头文件
├── tests/                     ← 单元 + 集成 + e2e 测试
├── third_party/               ← FetchContent 缓存（gitignore）
└── scripts/                   ← 辅助脚本
```

---

## 贡献

欢迎贡献！请先读 [CONTRIBUTING.md](./CONTRIBUTING.md)。

新人推荐找 `good-first-issue` 标签的 issue。

---

## License

[MIT](./LICENSE)

---

## 致谢

- [Anki](https://apps.ankiweb.net/) — 间隔重复算法的灵感
- [muduo](https://github.com/chenshuo/muduo) — 现代 C++ 服务端实践
- [典型 C++ 八股](https://github.com/<user>/bagu-docs)（作者另一个仓库）
