# bagu-cli

> 八股文档智能学习助手 — 用 C++ 写的命令行工具，帮你高效复习面试八股

## 这是什么

把零散的 markdown 八股文档（MySQL / Redis / C++ 网络等）变成一个可交互的复习系统：

- **智能复习** — 基于艾宾浩斯遗忘曲线推送复习题
- **全文搜索** — 模糊匹配快速找到任何知识点
- **交互式 TUI** — 终端里看题、答题、打分
- **AI 模拟面试** — 调 LLM API 让 AI 提问，自动评分
- **进度统计** — 哪些章节看过、薄弱在哪、刷题热力图

## 目标用户

- 准备面试的程序员（直接服务作者本人）
- 想用八股文档但又嫌 markdown 阅读累的人

## 当前状态

🚧 **开发中** — 项目刚启动，详细计划见 [docs/plan.md](./docs/plan.md)

## 快速开始

```bash
# 编译
mkdir build && cd build
cmake ..
make -j

# 导入八股文档
./bagu import /path/to/bagu-docs/

# 列出所有章节
./bagu list

# 进入交互复习模式
./bagu review

# 模糊搜索
./bagu search "MVCC 可见性"

# 调 AI 模拟面试
./bagu interview --topic mysql --num 5
```

## 技术栈

- **语言：** C++20
- **构建：** CMake 3.20+
- **存储：** SQLite3
- **CLI：** CLI11
- **TUI：** FTXUI
- **HTTP：** libcurl
- **JSON：** nlohmann/json
- **Markdown：** cmark
- **日志：** spdlog
- **测试：** GoogleTest

依赖通过 CMake `FetchContent` 自动下载，无需系统预装。

## 项目结构

```
bagu-cli/
├── CMakeLists.txt
├── README.md
├── docs/
│   ├── design.md       # 架构设计
│   └── plan.md         # 开发计划
├── src/
│   ├── main.cpp        # 入口
│   ├── cli/            # 命令行解析与各子命令
│   ├── db/             # SQLite 持久化层
│   ├── parser/         # Markdown 解析
│   ├── search/         # 全文搜索（倒排索引）
│   ├── tui/            # FTXUI 交互界面
│   ├── llm/            # LLM API 调用
│   ├── core/           # 核心业务（复习算法、评分）
│   └── util/           # 通用工具
├── include/bagu/       # 公共头文件
├── tests/              # 单元测试
├── third_party/        # 第三方库（FetchContent 拉取）
└── scripts/            # 辅助脚本
```

## License

MIT
