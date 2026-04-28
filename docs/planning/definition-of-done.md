# Definition of Done (DoD)

> **状态**：Approved
> **更新**：2026-04-29

---

## 0. 适用范围

DoD 适用于：
- 单个 PR
- 单个功能
- Sprint / Milestone
- 版本发布

不同层级的 DoD 不同，下面分别列。

---

## 1. PR 的 DoD

PR 可合入 main 必须满足：

### 代码
- [ ] 编译无 warning（GCC + Clang）
- [ ] 通过 clang-format
- [ ] 通过 clang-tidy 检查
- [ ] 无遗留 TODO（除非有 issue 跟踪）
- [ ] 无 `printf` / `cout` 调试代码
- [ ] 无注释掉的代码块
- [ ] 命名符合 [编码规范](../development/coding-standards.md)

### 测试
- [ ] 新增功能有单元测试
- [ ] 修复 bug 有回归测试
- [ ] 所有测试通过
- [ ] 覆盖率不低于该模块要求（见 [测试策略](../testing/strategy.md)）

### 文档
- [ ] 公开 API 有 doxygen 注释
- [ ] 复杂逻辑有 inline 注释（解释 why）
- [ ] 用户可见变更已加入 CHANGELOG.md（Unreleased）
- [ ] 重大设计变更有 ADR

### 提交
- [ ] commit 信息符合 [conventional commits](../development/git-workflow.md)
- [ ] 单 PR 改动 < 500 行（超过要拆）
- [ ] PR 描述完整（这是什么 / 为什么 / 怎么做 / 测试）
- [ ] 自我 review 一遍 diff

### CI
- [ ] CI 全绿
- [ ] benchmark 无明显回退（< 10%）
- [ ] 覆盖率达标

### Review
- [ ] 自审
- [ ] （如有合作者）至少 1 位 approval

---

## 2. 功能（Feature）的 DoD

一个完整功能上线必须满足：

### 设计
- [ ] 有需求描述（issue 或 PRD section）
- [ ] 有设计方案（在 LLD 文档或 PR 描述中）
- [ ] 重大决策有 ADR

### 实现
- [ ] 主要 happy path 实现
- [ ] 至少 2 个 edge case 处理
- [ ] 错误路径返回正确错误码
- [ ] 日志覆盖关键节点

### 测试
- [ ] 单元测试（核心逻辑）
- [ ] 集成测试（与其他模块的交互）
- [ ] 至少 1 个 e2e 场景验证

### 文档
- [ ] CLI 规范更新（如有新命令）
- [ ] 模块 LLD 更新
- [ ] CHANGELOG 加条目

### 用户体验
- [ ] 命令有 `--help` 输出
- [ ] 错误信息清晰可操作
- [ ] 性能符合预算

---

## 3. Sprint 的 DoD

Sprint 结束（每周五 EOD）：

- [ ] 本 Sprint 的所有任务关闭或推到下个 Sprint
- [ ] 所有合入的代码符合 PR DoD
- [ ] 演示可运行（`./bagu xxx` 跑通）
- [ ] 写 retrospective（哪里好哪里差）
- [ ] 更新 [Milestones](./milestones.md) 表格

---

## 4. 版本（Release）的 DoD

发版前必须满足：

### 代码
- [ ] 所有计划功能合入
- [ ] 所有 P0/P1 bug 修复
- [ ] 已知 P2/P3 bug 在 CHANGELOG 中明示

### 质量
- [ ] 覆盖率达 [测试策略](../testing/strategy.md) 中的目标
- [ ] 性能基准达 [性能预算](../quality/performance-budget.md)
- [ ] 通过 [安全清单](../quality/security-checklist.md) 自查
- [ ] CI 在 main 上连续 3 天绿色

### 兼容性
- [ ] 数据库 schema migration 测试
- [ ] 配置文件兼容（升级用户能正常用）
- [ ] CLI 命令向后兼容（除 MAJOR 升级）

### 文档
- [ ] CHANGELOG 完整
- [ ] README 版本号 / 安装说明 / 截图更新
- [ ] 重大变更有迁移指南
- [ ] 文档无 broken link

### 发布产物
- [ ] 多平台二进制构建成功（CI）
- [ ] tar.gz / .deb / brew formula 测试可装
- [ ] GitHub Release 页面完整
- [ ] tag 已推送

### 验证
- [ ] 在干净环境重装测试
- [ ] 旧版本升级测试
- [ ] 至少 1 位外部用户验证（v1.0+）

---

## 5. 技术债务的 DoD

技术债务关闭：

- [ ] 实际重构完成
- [ ] 测试通过
- [ ] 性能未回退
- [ ] 文档更新

---

## 6. Bug 的 DoD

bug 修复关闭：

- [ ] 复现步骤已验证
- [ ] 根因已找到
- [ ] 修复代码合入
- [ ] **回归测试已加**（重要！）
- [ ] CHANGELOG 中记录

---

## 7. 文档的 DoD

文档变更：

- [ ] 内容准确（与代码一致）
- [ ] 链接可达
- [ ] 代码示例可运行
- [ ] 格式正确（markdown lint 过）
- [ ] 文档索引（README）已更新

---

## 8. 反 DoD（不算完成）

下面这些都不算 done：

- ❌ "在我机器上能跑"
- ❌ "测试还没写但代码改完了"
- ❌ "CI 失败但不是我引起的"
- ❌ "写完代码就 push，没自审 diff"
- ❌ "改完没更新文档"
- ❌ "warn 暂时不修"

---

## 9. 检查清单（PR 模板用）

```markdown
## Definition of Done

### 代码
- [ ] 编译无 warning
- [ ] clang-format 通过
- [ ] 命名符合规范
- [ ] 无调试代码遗留

### 测试
- [ ] 新增功能有单测
- [ ] CI 通过
- [ ] 覆盖率达标

### 文档
- [ ] 公开 API 有注释
- [ ] CHANGELOG 已更新（用户可见变更）

### Review
- [ ] 自审一遍 diff
```

---

## 10. 相关文档

- [测试策略](../testing/strategy.md)
- [性能预算](../quality/performance-budget.md)
- [安全清单](../quality/security-checklist.md)
- [Git 工作流](../development/git-workflow.md)
