# 产品路线图（Roadmap）

> **状态**：Approved
> **更新**：2026-04-29
> **下次 review**：2026-05-31

---

## 总览

```
2026   M5         M6         M7         M8         M9         Q4
       │          │          │          │          │          │
v0.1   ●━━━━━━━━━│          │          │          │          │
       MVP                                                     │
v0.2              ●━━━━━━━━│          │          │          │
                  AI 面试 + 统计                                │
v0.3                         ●━━━━━━━━│          │          │
                             性能 + 测试                        │
v1.0                                    ●━━━━━━━━│          │
                                        公开发布                │
v1.x                                              ●━━━━━━━━━━━│
                                                  插件/扩展      │
v2.0                                                            ●
                                                                Web UI?
```

---

## Q2 2026 (Apr-Jun)：MVP & 公开发布

### v0.1.0 — MVP（目标 2026-05-19）

**主题：** 跑起来 + 自用

**功能：**
- ✅ `init` / `import` / `list` / `search` / `review`
- ✅ Markdown 解析
- ✅ SM-2 复习算法
- ✅ FTXUI 复习界面
- ✅ SQLite + FTS5 搜索

**质量门槛：**
- 单元测试覆盖率 ≥ 70%
- 三份八股文档（mysql/redis/cpp-net）能完整跑通
- 自己每天能用

### v0.2.0 — AI 面试 + 统计（目标 2026-05-26）

**主题：** 智能化

**功能：**
- ✅ `interview` 命令（OpenAI/Claude/Ollama）
- ✅ `stats` 命令
- ✅ `stats --heatmap`
- ✅ `config` 命令
- ✅ `show <id>` 命令

### v0.3.0 — 质量与性能（目标 2026-06-09）

**主题：** 工业级

**功能：**
- ✅ 完整 e2e 测试
- ✅ benchmark 集成 CI
- ✅ 覆盖率 ≥ 85%
- ✅ 完整文档

**性能：**
- 启动 < 50ms
- 搜索 < 100ms
- 导入 1000 cards/s

### v1.0.0 — 公开发布（目标 2026-06-30）

**主题：** 让别人能用

**功能：**
- ✅ 完整 README + 截图 + GIF
- ✅ Homebrew formula
- ✅ Debian package
- ✅ GitHub Actions release pipeline
- ✅ CHANGELOG 规范
- ✅ CONTRIBUTING / CODE_OF_CONDUCT
- ✅ Issue / PR 模板

**发布渠道：**
- GitHub Release
- HackerNews / Reddit r/cpp
- 知乎 / CSDN / 掘金博客

---

## Q3 2026 (Jul-Sep)：扩展与生态

### v1.1.0 — 用户体验（目标 2026-07-31）

- 颜色主题
- 富文本 markdown 渲染（代码高亮）
- 导出 Anki 牌组
- `bagu remind` 系统通知提醒
- 多 profile 支持（区分场景）

### v1.2.0 — 协作（目标 2026-08-31）

- `bagu sync` 加密同步到 GitHub Gist
- 公共题库订阅（社区维护）
- 收藏 / 评论卡片

### v1.3.0 — 高级搜索（目标 2026-09-30）

- 中文分词（cppjieba）
- 相似题目推荐（embedding）
- 智能标签（LLM 提取）

---

## Q4 2026 (Oct-Dec)：v2.0 探索

### v2.0.0-beta — Web UI（目标 2026-11-30）

- 可选 Web 界面（基于 crow）
- 适合不擅长终端的用户
- 但仍以 CLI 为主

### v2.0.0 — 重大重构

- 数据 schema v2
- 协程化（C++20 coroutine）
- 插件机制

---

## 持续做的事

### 每个迭代都要做
- 修 bug
- 重构 / 重命名
- 升级依赖
- 完善文档

### 每月做
- Performance review
- 用户反馈 review
- Roadmap 调整

---

## 已知不做（明确否决）

| 不做                | 理由                                         |
|------------------|--------------------------------------------|
| 移动端 App          | 偏离 CLI 工具定位                              |
| SaaS / 多租户        | 个人工具                                      |
| 内容生产平台         | 用户写八股，本工具只读                          |
| 商业化            | 完全开源免费                                  |
| 强账号体系         | 单机即可                                      |
| 自研 LLM            | 调外部 API 即可                                |

---

## 反馈收集

- GitHub Issues
- 自己使用反馈
- 朋友使用反馈

每两周整理 → 加入 backlog → 排优先级。

---

## 度量

每个版本发布后 1 周内统计：
- 下载量
- 新 issues
- 严重 bugs
- 性能跑分

---

## 相关文档

- [PRD](../product/PRD.md)
- [里程碑](./milestones.md)
- [风险登记册](./risks.md)
