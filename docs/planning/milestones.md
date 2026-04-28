# 里程碑（Milestones）

> **状态**：Approved
> **更新**：2026-04-29

---

## M1 — Sprint 1：项目骨架（4/29 - 5/5）

### 目标
工程跑通：编译 → 配置 → 数据库 → 一个最简单命令。

### 任务
- [x] 项目目录结构
- [x] CMakeLists.txt + FetchContent
- [x] 文档体系
- [ ] 配置模块（toml++）
- [ ] SQLite 封装层
- [ ] `bagu init` 命令实际逻辑
- [ ] `bagu config get/set` 实际逻辑
- [ ] 单元测试框架（GTest）
- [ ] 至少 5 个单测

### Definition of Done（DoD）
见 [DoD](./definition-of-done.md)。

特别要求：
- ✅ `cmake -B build && cmake --build build` 一次成功
- ✅ `./bagu --version` 输出正确
- ✅ `./bagu init` 创建 `~/.bagu/bagu.db`
- ✅ ctest 全绿
- ✅ Git 提交规范

---

## M2 — Sprint 2：导入与浏览（5/6 - 5/12）

### 目标
能把 markdown 文档变成可查询的卡片库。

### 任务
- [ ] 接入 cmark
- [ ] Markdown 解析器（提取 topic / chapter / card）
- [ ] `bagu import <path>` 命令
- [ ] `bagu list` / `bagu list <topic>` 命令
- [ ] `bagu show <id>` 命令
- [ ] `bagu search <keyword>` 命令（基础版）
- [ ] FTS5 索引
- [ ] 中文分词（N-gram 简版）
- [ ] 集成测试：导入 → 查询 → 删除

### DoD
- ✅ 导入现有 mysql-interview.md 不报错
- ✅ `list` 显示 10 个章节
- ✅ `search "MVCC"` 至少返回 1 条
- ✅ 单元测试覆盖率 ≥ 60%
- ✅ 解析器有完整 e2e 测试

### 演示场景
```bash
bagu import /home/ly/bagu-docs/
# 输出：3 topics, 245 cards imported

bagu list
# 输出：mysql/redis/cpp-net 列表

bagu search "MVCC"
# 输出：3 张相关卡片
```

---

## M3 — Sprint 3：复习模式（5/13 - 5/19）

### 目标
完整的 SM-2 复习闭环 + TUI 交互。

### 任务
- [ ] 实现 SM-2 算法（src/core/sm2.cpp）
- [ ] 卡片选择策略（CardPicker）
- [ ] 复习历史记录
- [ ] 接入 FTXUI
- [ ] 复习 TUI 主界面
- [ ] 按键交互（1-5 评分、空格揭示、q 退出）
- [ ] 进度持久化
- [ ] 撤销上次评分
- [ ] 中断处理（Ctrl+C 优雅退出）

### DoD
- ✅ SM-2 单测覆盖率 ≥ 95%
- ✅ TUI 在 80 列宽终端正常显示
- ✅ 评分后 next_review 计算正确
- ✅ 至少 1 个完整 e2e 流程通过

### 演示场景
```bash
bagu review --topic mysql --num 10
# 进入 TUI，复习 10 张
# 退出后显示：复习了 10 张，正确 8 张
```

### v0.1.0 发布！

---

## M4 — Sprint 4：AI 面试 + 统计（5/20 - 5/26）

### 目标
LLM 集成 + 学习统计。

### 任务
- [ ] LLM 抽象接口
- [ ] OpenAI 客户端
- [ ] Claude 客户端
- [ ] Ollama 客户端（本地）
- [ ] SSE 流式响应
- [ ] `bagu interview` 命令
- [ ] 面试会话记录
- [ ] `bagu stats` 命令
- [ ] `bagu stats --heatmap` 热力图

### DoD
- ✅ 可切换不同 LLM provider
- ✅ 网络异常优雅降级
- ✅ API key 严格走 env
- ✅ 热力图在 80 列宽终端正常显示

### v0.2.0 发布！

---

## M5 — Sprint 5：质量与发布（5/27 - 6/30）

### 目标
v1.0 公开发布。

### 任务
- [ ] 完善单元测试 → 覆盖率 ≥ 85%
- [ ] e2e 测试套件
- [ ] benchmark 集成 CI
- [ ] 性能优化（按 perf 火焰图）
- [ ] CI matrix（Linux/Mac × GCC/Clang）
- [ ] CHANGELOG 完整
- [ ] CONTRIBUTING 完整
- [ ] README 完整（含截图、GIF）
- [ ] Homebrew formula
- [ ] Debian package
- [ ] GitHub Release pipeline
- [ ] 写技术博客

### v1.0.0 发布！

---

## 风险跟踪

每个里程碑结束做回顾：
- 实际 vs 计划进度
- 阻塞项
- 学到了什么
- 下个里程碑调整

---

## 度量

| 里程碑 | 计划日期 | 实际日期 | 进度 | 备注 |
|------|--------|--------|------|----|
| M1   | 5/5    | TBD    | -    |    |
| M2   | 5/12   | TBD    | -    |    |
| M3   | 5/19   | TBD    | -    |    |
| M4   | 5/26   | TBD    | -    |    |
| M5   | 6/30   | TBD    | -    |    |

---

## 相关文档

- [Roadmap](./roadmap.md)
- [DoD](./definition-of-done.md)
- [Risks](./risks.md)
