# 产品路线图（Roadmap）

> **状态**：Approved
> **更新**：2026-05-03（v1.2 已发）
> **下次 review**：2026-07-01

> ✅ **v1.0.0**（2026-05-03）— Homebrew / CI matrix / 用户手册
> ✅ **v1.1.0**（2026-05-03 同日）— Anki 导出 / 稳定 card.id / Web 历史 / 多 profile
> ✅ **v1.2.0**（2026-05-03 同日）— 「今日到期」speed view + `bagu remind` 桌面通知
> ✅ **v1.2.1**（2026-05-03 同日）— CSV 导出 + 卡片 pause/unpause + LAN IP 打印 + 搜索引导 + runbook

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
v0.3                                    ●━━━━━━━━│          │
                                        Web UI MVP              │
v0.4                                              ●━━━━━━━━━━━│
                                                  Web UI 完善    │
v1.0                                                            ●
                                                          公开发布 + Homebrew + brew
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

### v1.0.0 — 公开发布 ✅ 2026-05-03（提前 ~2 个月）

**主题：** 让别人能用

**已交付：**
- ✅ README + 用户手册 6 篇
- ✅ Homebrew tap（`chunluren/homebrew-bagu`）
- ✅ GitHub Actions release pipeline（matrix: linux-x86_64 + macos-arm64）
- ✅ CHANGELOG 规范（Keep a Changelog）
- ✅ CI 5 组合：ubuntu-22/24 × gcc/clang + macos-14
- ✅ 192 自动化测试（171 单测 + 21 e2e）
- ✅ HTTP server 多线程并发安全（FULLMUTEX）

**未交付（推迟到 v1.x）：**
- Debian package（影响小，下次再做）
- CONTRIBUTING.md / CODE_OF_CONDUCT / Issue 模板（项目主要单人维护，按需补）
- 截图 / GIF（README 文字够用；后续 hackernews 投放再做）

**发布渠道：**
- GitHub Release
- HackerNews / Reddit r/cpp
- 知乎 / CSDN / 掘金博客

---

### v0.3.0 — Web UI MVP（目标 2026-05-15）

- `bagu serve` HTTP 服务模式（cpp-httplib）
- REST API（涵盖现有 CLI 读路径）
- React + Vite + Tailwind 前端 SPA
- 4 个页面：主题列表 / 章节树 / 卡片详情 / 搜索 / 复习
- 默认 127.0.0.1 / 可选 token 鉴权
- 详见 [PRD-web-ui.md](../product/PRD-web-ui.md)

### v0.4.0 — Web UI 完善（目标 2026-05-31）

- AI 模拟面试页（SSE 流式渲染）
- 学习统计页 + 热力图
- PWA 支持（加到主屏）
- 暗黑/明亮主题
- 移动端适配

---

## Q3 2026 (Jul-Sep)：扩展与生态

### v1.1.0 — 用户体验扩展 ✅ 2026-05-03（提前 ~3 个月）

**已交付：**
- ✅ 导出 Anki 牌组（`bagu export anki` + `/api/export/anki`）
- ✅ 多 profile 支持（`[llm.profiles.<name>]` + `--profile`）
- ✅ 稳定 card.id（重导入保留 SM-2 复习历史，schema v4）
- ✅ Web 面试历史页（`/interview/history` + `/interview/sessions/:id`）

**推迟到 v1.3+：**
- 中文分词（cppjieba，需评估字典体积 ~10MB）
- 相似题目推荐（embedding，需 vector 存储 + LLM API）

---

### v1.2.0 — 今日复习推送 ✅ 2026-05-03

**已交付：**
- ✅ `bagu due` / `GET /api/review/due-summary` / Web 首页 hero banner
- ✅ `bagu remind` 桌面通知（Linux/macOS）+ cron/systemd/launchd 文档

### v1.2.0 — 协作（目标 2026-08-31）

- `bagu sync` 加密同步到 GitHub Gist
- 公共题库订阅（社区维护）
- 收藏 / 评论卡片

---

## Q4 2026 (Oct-Dec)：v2.0 探索

### v2.0.0 — 重大重构

- 数据 schema v2
- 协程化（C++20 coroutine）
- 插件机制
- Tauri 桌面 App 评估

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
