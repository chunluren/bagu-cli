# 贡献指南

感谢有兴趣为 bagu-cli 贡献！

> 在贡献之前，请先读 [架构总览](./docs/architecture/overview.md) 与 [编码规范](./docs/development/coding-standards.md)。

---

## 快速开始

```bash
# 1. fork 仓库
gh repo fork <owner>/bagu-cli --clone

# 2. 装依赖
cd bagu-cli
sudo apt install -y build-essential cmake ninja-build libsqlite3-dev libcurl4-openssl-dev

# 3. 构建测试
mkdir build && cd build
cmake -G Ninja -DCMAKE_BUILD_TYPE=Debug ..
ninja
ctest --output-on-failure

# 4. 开新分支
git checkout -b feature/my-improvement

# 5. 修改代码 + 提交
git add ...
git commit -m "feat(xxx): describe what"

# 6. 推送 + 开 PR
git push -u origin feature/my-improvement
gh pr create
```

---

## 贡献类型

| 类型         | 难度  | 入门推荐 |
|------------|-----|------|
| 文档改进         | ★    | ✅    |
| 修复拼写 / typo  | ★    | ✅    |
| 修复小 bug       | ★★   | ✅    |
| 加单元测试        | ★★   | ✅    |
| 新功能（小）       | ★★★  |      |
| 性能优化          | ★★★★ |      |
| 重大架构变更        | ★★★★★|      |

**新人推荐：** 找带 `good-first-issue` 标签的 issue。

---

## 提交流程

### 1. 找 issue 或 开 issue
- 大改动先开 issue 讨论
- 小改动可直接 PR

### 2. 设计阶段
- 重大变更需要 [ADR](./docs/architecture/adr/template.md)
- 在 issue 里贴方案，先达成共识

### 3. 编码阶段
- 遵循 [编码规范](./docs/development/coding-standards.md)
- 遵循 [Git 工作流](./docs/development/git-workflow.md)
- 用 [conventional commits](./docs/development/git-workflow.md#2-commit-规范conventional-commits)

### 4. 测试
- 新功能必须有测试
- bug 修复必须有回归测试
- 跑 `ctest` 全绿

### 5. 文档
- 新增 / 改动公开 API 必须有 doxygen 注释
- 用户可见的变更加入 CHANGELOG.md

### 6. PR
- 标题用 conventional commits
- 描述用 PR 模板填完整
- 自审一遍 diff
- 等 CI 跑过

### 7. Review
- 耐心回应反馈
- 不抗拒被批评
- 改完 push，会自动重跑 CI

---

## PR 模板

```markdown
## 这是什么？
（简要描述本 PR 做了什么）

## 为什么需要？
（背景、问题、需求；链接相关 issue）

## 怎么做的？
（核心实现思路、关键决策）

## 测试
- [ ] 单元测试已加
- [ ] CI 通过
- [ ] 手动测试场景：xxx

## DoD
- [ ] 编译无 warning
- [ ] clang-format 通过
- [ ] 测试覆盖率达标
- [ ] CHANGELOG 更新（用户可见）
- [ ] 文档更新

## 相关
Closes #123
```

---

## 代码风格

- 详见 [编码规范](./docs/development/coding-standards.md)
- 自动格式化：`clang-format -i src/**/*.{cpp,h}`

---

## Commit 规范

```
<type>(<scope>): <subject>

<body>

<footer>
```

- type: feat / fix / docs / refactor / perf / test / build / ci / chore / revert
- scope: 模块名（cli / db / parser / ...）
- subject: 50 字内，现在时

例：
```
feat(parser): support code blocks in markdown

Previously code blocks were treated as plain text.
Now extract them as separate cards with type="code".

Closes #42
```

---

## 行为准则

我们采用 [Contributor Covenant Code of Conduct](https://www.contributor-covenant.org/version/2/1/code_of_conduct/)。

简单说：**互相尊重、不歧视、对事不对人**。

---

## License

提交即同意你的代码以 [MIT License](./LICENSE) 发布。

---

## 联系

- Issues: https://github.com/<owner>/bagu-cli/issues
- Discussions: https://github.com/<owner>/bagu-cli/discussions

---

## 给项目作者的话

如果你 fork 了项目并觉得有用，**给个 star** 是最大的鼓励。
