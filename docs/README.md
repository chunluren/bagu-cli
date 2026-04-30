# bagu-cli 文档中心

> 项目所有的设计、规范、计划、运维文档

## 文档索引

### 📋 产品（product/）
- [PRD - 产品需求文档](./product/PRD.md)
- [PRD-web-ui - Web UI 需求（v0.3 计划）](./product/PRD-web-ui.md)
- [用户画像](./product/personas.md)

### 🏗 架构（architecture/）
- [架构总览（HLD）](./architecture/overview.md)
- [数据模型](./architecture/data-model.md)
- [CLI 命令规范](./architecture/cli-spec.md)
- [模块详细设计（LLD）](./architecture/modules.md)
- [Web UI 架构（v0.3）](./architecture/web-ui-overview.md)
- [HTTP API 规范](./architecture/http-api-spec.md)
- [架构决策记录（ADR）](./architecture/adr/README.md)

### 💻 开发（development/）
- [开发环境搭建](./development/setup.md)
- [C++ 编码规范](./development/coding-standards.md)
- [Git 工作流](./development/git-workflow.md)
- [调试指南](./development/debugging.md)

### 🧪 测试（testing/）
- [测试策略](./testing/strategy.md)
- [性能基准](./testing/benchmarks.md)

### 🚀 运维（operations/）
- [构建说明](./operations/build.md)
- [CI/CD 流程](./operations/ci-cd.md)
- [发布流程](./operations/release.md)
- [日志规范](./operations/logging.md)
- [错误处理与错误码](./operations/error-handling.md)

### 🛡 质量（quality/）
- [性能预算](./quality/performance-budget.md)
- [安全清单](./quality/security-checklist.md)

### 📅 规划（planning/）
- [产品路线图（Roadmap）](./planning/roadmap.md)
- [Web UI 实施路线图（v0.3-v0.4）](./planning/web-ui-roadmap.md)
- [里程碑](./planning/milestones.md)
- [风险登记册](./planning/risks.md)
- [完成定义（DoD）](./planning/definition-of-done.md)

### 📚 参考（reference/）
- [术语表](./reference/glossary.md)

### 📜 顶层文件
- [CHANGELOG.md](../CHANGELOG.md) — 版本变更日志
- [CONTRIBUTING.md](../CONTRIBUTING.md) — 贡献指南

---

## 文档维护原则

1. **Living docs** — 文档与代码同步更新，过时文档比无文档更糟
2. **Single source of truth** — 每个事实只有一处记录，其他地方引用
3. **Code as documentation** — 能用代码 / 类型表达的不写文档
4. **简洁优先** — 一页能讲清的不写两页
5. **新增重大决策必须写 ADR**

## 谁负责什么

| 文档类型     | 维护者         | 更新频率                |
|------------|---------------|------------------------|
| PRD        | 产品 / 作者    | 重要变更时              |
| 架构 / ADR | 技术负责人     | 重大决策时              |
| 模块 LLD   | 模块作者       | 模块变更时              |
| 编码规范    | 全员          | 季度 review            |
| 路线图      | 产品 / 作者    | 月度 review            |
| 风险登记册  | 技术负责人     | 双周 review            |
| CHANGELOG  | 发版者         | 每次发版                |

## 文档模板

新文档基于以下模板：

```markdown
# 标题

> **状态**：Draft / Review / Approved / Deprecated
> **作者**：xxx
> **创建日期**：YYYY-MM-DD
> **最后更新**：YYYY-MM-DD
> **版本**：x.y.z

## 背景 / 目的
...

## 内容
...

## 相关文档
- [link]
```
