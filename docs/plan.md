# bagu-cli 开发计划（4 周）

> 起始：2026-04-29
> 目标：4 周内完成 MVP，可投入日常使用

---

## Week 1（4/29 - 5/5）：项目骨架

### 目标
搭好工程，实现最简单的命令链路。

### 任务

**Day 1（4/29 周二）**
- [x] 创建项目目录结构
- [ ] 写 CMakeLists.txt 主框架
- [ ] FetchContent 接入 CLI11 / spdlog / nlohmann/json / SQLiteCpp
- [ ] 验证编译通过

**Day 2（4/30 周三）**
- [ ] 实现 main.cpp 入口
- [ ] 命令分发：`bagu version` / `bagu help`
- [ ] 集成 spdlog 日志系统

**Day 3（5/1 周四）**
- [ ] 配置模块：读取 ~/.bagu/config.toml（用 toml++）
- [ ] `bagu init`：建数据目录、复制默认配置、初始化 db schema
- [ ] `bagu config get/set`

**Day 4（5/2 周五）**
- [ ] SQLite 封装层（src/db/）
- [ ] schema 创建脚本
- [ ] DAO 基础（topic / chapter / card）

**Day 5-7（5/3-5/5）**
- [ ] 单元测试框架（GoogleTest）
- [ ] 测试 db / config 模块
- [ ] 完成 README 的"快速开始"部分能跑通

**周末验收：**
```bash
$ bagu init
$ bagu config get docs_dir
$ bagu config set llm.provider claude
```

---

## Week 2（5/6 - 5/12）：导入与浏览

### 目标
能把 markdown 八股导入数据库，并支持列表 + 搜索。

### 任务

**Day 8-9**
- [ ] 接入 cmark，实现 markdown 解析器
- [ ] 解析策略：识别 `## 第 N 章` 为章节，`**Q数字.**` 抽取题目
- [ ] 单元测试：用现有的 mysql-interview.md 验证

**Day 10**
- [ ] `bagu import <path>`：递归读 md 文件，入库
- [ ] 处理重复导入（按文件 hash 判断）

**Day 11**
- [ ] `bagu list`：树形显示主题 → 章节 → 题数
- [ ] 命令 `bagu list mysql` 显示 mysql 的章节
- [ ] `bagu show <card_id>` 看单题

**Day 12-13**
- [ ] 启用 SQLite FTS5
- [ ] `bagu search "MVCC"`
- [ ] 中文分词（用 cppjieba 或 N-gram 简单实现）

**Day 14**
- [ ] 整理输出格式（彩色 + 高亮匹配关键词）
- [ ] 写 e2e 测试脚本

**周末验收：**
```bash
$ bagu import /home/ly/bagu-docs/
Imported 3 topics, 245 cards.

$ bagu list
mysql      (87 cards)  10 chapters
redis      (87 cards)  11 chapters
cpp-net    (85 cards)  12 chapters

$ bagu search "MVCC"
[mysql/4.4] MVCC 三大组件
[mysql/Q43] MVCC 怎么实现？
[mysql/Q44] Read View 可见性规则？
...
```

---

## Week 3（5/13 - 5/19）：复习模式

### 目标
基于 SM-2 算法的复习系统 + TUI 交互。

### 任务

**Day 15-16**
- [ ] 实现 SM-2 复习算法（src/core/sm2.cpp）
- [ ] 单元测试覆盖各种 score 路径
- [ ] `bagu review --plan` 显示今日待复习题数

**Day 17-18**
- [ ] 接入 FTXUI
- [ ] 复习 TUI 主界面（题目卡片、按键提示）
- [ ] 按键 1-5 评分 → 调用 SM-2 更新

**Day 19**
- [ ] 复习历史落库（review_history 表）
- [ ] `bagu review --topic mysql --num 10`
- [ ] 退出时保存进度

**Day 20**
- [ ] 新卡片调度（每天最多 N 张新卡）
- [ ] 难度分布显示

**Day 21**
- [ ] 优化 TUI 响应（减少重绘）
- [ ] 增加快捷键（j/k 翻页、空格揭示答案）
- [ ] 截图录制到 README

**周末验收：**
```bash
$ bagu review
# 进入 TUI，开始今日复习
# 完成后显示：复习了 18 题，正确 14，提升曲线...
```

---

## Week 4（5/20 - 5/26）：AI 面试 + 统计

### 目标
集成 LLM API，实现 AI 模拟面试 + 统计可视化。

### 任务

**Day 22-23**
- [ ] LLM 客户端抽象（src/llm/）
- [ ] OpenAIClient（libcurl + JSON）
- [ ] ClaudeClient
- [ ] OllamaClient（本地模型）
- [ ] SSE 流式输出（让回答边出边显示）

**Day 24-25**
- [ ] 面试模式 prompt 模板
- [ ] `bagu interview --topic mysql --num 5`
- [ ] AI 提问 → 用户答 → AI 评分 + 反馈
- [ ] 会话历史存到 interview_session 表

**Day 26**
- [ ] `bagu stats` 基础统计
   - 总题数、已学、正确率、连续天数
- [ ] 各 topic 进度
- [ ] 薄弱知识点（最近答错最多的）

**Day 27**
- [ ] `bagu stats --heatmap` GitHub 风格热力图
   - 用 ASCII 字符或 unicode block
   - 显示最近 90 天的复习活动

**Day 28**
- [ ] 文档：使用手册 / FAQ / 截图
- [ ] release v0.1：打 tag，准备发 GitHub
- [ ] 写一篇 CSDN / 掘金博客介绍项目

**最终验收：**
```bash
$ bagu interview --topic mysql --num 3
[AI] 第 1 题：MVCC 是怎么实现的？
[你] (输入答案，多行 Ctrl+D 结束)
> ...
[AI] 评分 7/10
[AI] 反馈：你答到了隐藏字段和 undo log，但没说 Read View 可见性规则...
...

$ bagu stats --heatmap
        Apr   May   Jun
Mon     ▁ ▃ ▆ █ ▇ ▅ ...
Tue     ▂ ▄ █ ▆ ▃ ...
...
```

---

## 持续做的事（每天）

- 提交代码（每天至少 1 个 commit）
- 写 commit message 用 conventional commits（feat/fix/docs/refactor/test）
- README 跟进项目进度
- 运行测试不能挂

---

## Stretch Goals（如果时间充裕）

- [ ] **导出**：`bagu export anki` 导出为 Anki 牌组
- [ ] **TUI 浏览模式**：在 TUI 里浏览所有 card，支持搜索/收藏
- [ ] **图片支持**：解析 markdown 中的代码块和示意图
- [ ] **多用户**：支持多个 profile（区分自己用 vs 帮朋友用）
- [ ] **定时提醒**：systemd timer 每天提醒复习
- [ ] **数据云同步**：可选的同步到 GitHub Gist（加密）
- [ ] **Web UI**：基于 crow 框架的本地网页（可选，超工作量）

---

## 风险与应对

| 风险                       | 应对                                  |
|--------------------------|--------------------------------------|
| FTXUI 学习成本             | 先做最简单文本输出，TUI 放 Week 3 末再加  |
| Markdown 解析复杂度         | 先按规则提取，复杂结构后续优化            |
| LLM API key 没钱           | 改用本地 Ollama（免费）                 |
| 中文分词麻烦                | 先用 N-gram（每 2 字一组）兜底           |
| 时间不够                    | Week 4 砍掉 stats，只保留 interview    |

---

## 衡量成功的指标

**MVP 成功标准（必须达成）：**
- ✅ 能正确导入 mysql/redis/cpp-net 三份文档
- ✅ `bagu review` 能跑起来，每天能用
- ✅ `bagu search` 秒级返回
- ✅ 至少 80% 单元测试覆盖率
- ✅ README 完整

**可选目标：**
- 🎯 GitHub 上 50+ star
- 🎯 写一篇技术博客 1000+ 阅读
- 🎯 在面试时被面试官问到这个项目并展开聊
