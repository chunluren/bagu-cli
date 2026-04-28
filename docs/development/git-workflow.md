# Git 工作流

> **状态**：Approved
> **更新**：2026-04-29

---

## 1. 分支模型（GitHub Flow 简化版）

```
main          ────●────●────●────●────●────  (永远可发布)
                   ↑    ↑    ↑    ↑    ↑
                 PR   PR   PR   PR   PR
                  ╱    ╱    ╱    ╱    ╱
feature/xxx   ──●─────●─────●─────●─────●─    (开发分支)
```

### 主分支

- **main** — 唯一长期分支，始终保持可发布状态
- 直接 push 受保护，必须通过 PR

### 开发分支命名

| 前缀         | 用途             | 示例                               |
|------------|----------------|----------------------------------|
| `feature/` | 新功能           | `feature/import-cmd`             |
| `fix/`     | bug 修复          | `fix/db-connection-leak`         |
| `refactor/` | 重构             | `refactor/extract-card-service`  |
| `docs/`    | 文档             | `docs/update-roadmap`            |
| `test/`    | 测试相关          | `test/add-sm2-tests`             |
| `chore/`   | 杂项（CI、依赖）   | `chore/upgrade-cmake`            |
| `perf/`    | 性能优化          | `perf/optimize-search`           |

---

## 2. Commit 规范（Conventional Commits）

### 2.1 格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### 2.2 type

| type     | 说明                            |
|--------|-------------------------------|
| feat   | 新功能                           |
| fix    | bug 修复                         |
| docs   | 文档变更                          |
| style  | 不影响逻辑的格式化（空格、换行）         |
| refactor | 重构（既不是 feat 也不是 fix）    |
| perf   | 性能优化                          |
| test   | 测试相关                          |
| build  | 构建系统 / 外部依赖变更               |
| ci     | CI 配置变更                       |
| chore  | 杂务（如更新依赖）                    |
| revert | 回滚之前的 commit                   |

### 2.3 scope（可选）

模块名或子系统：`cli`、`db`、`parser`、`tui`、`docs`...

### 2.4 subject

- 50 字符内
- 现在时（"add"，不是 "added"）
- 首字母小写
- 不加句号

### 2.5 body（可选）

- 解释 **为什么**，不解释做了什么（git diff 已经显示）
- 多段用空行分隔
- 行宽 72

### 2.6 footer（可选）

- BREAKING CHANGE: ...
- Closes #123

### 2.7 示例

**简单：**
```
feat(cli): add 'bagu show <id>' command
fix(db): handle connection timeout properly
docs: update README with installation steps
chore: bump CLI11 to v2.4.2
```

**复杂：**
```
feat(parser): support extracting QA from question bank chapters

Previously parser only extracted full sections as cards.
Now also recognizes the **Q1. ?** answer pattern in 第 11 章 题库.

This dramatically improves card quality - QA pairs are more
focused than whole sections.

Closes #15
```

**破坏性变更：**
```
refactor(db)!: rename Card.body to Card.answer

BREAKING CHANGE: Card.body 字段重命名为 Card.answer
所有依赖该字段的代码需要修改

Closes #42
```

---

## 3. 提交流程

### 3.1 开发新功能

```bash
# 1. 从 main 拉新分支
git checkout main
git pull
git checkout -b feature/my-feature

# 2. 开发 + 提交（小步快跑）
# 写代码...
git add src/...
git commit -m "feat(xxx): add ..."

# 多次提交都可以，最后会 squash
# 写代码...
git add ...
git commit -m "test(xxx): add tests"

# 3. 推送
git push -u origin feature/my-feature

# 4. 在 GitHub 开 PR
gh pr create --title "..." --body "..."

# 5. CI 跑过 + review 通过 → squash merge

# 6. 删除本地分支
git checkout main
git pull
git branch -d feature/my-feature
```

### 3.2 修 bug

```bash
git checkout main && git pull
git checkout -b fix/something-broken

# 修复 + 测试
git commit -m "fix(xxx): handle null case"

git push -u origin fix/something-broken
gh pr create --label bug
```

### 3.3 紧急修复（hotfix）

如果有正式版本：
```bash
git checkout v1.0.0
git checkout -b hotfix/critical-issue
# 修复
git commit -m "fix: critical security issue"
# merge 回 main 同时打新 tag
```

---

## 4. PR 规范

### 4.1 标题
跟 commit 规范一致：
```
feat(cli): add 'bagu show' command
```

### 4.2 描述模板

```markdown
## 这是什么？
简要描述本 PR 的目的。

## 为什么需要？
背景、问题、需求。

## 怎么做的？
关键设计决策。

## 测试
- [ ] 单元测试已加
- [ ] 手动测试场景：xxx
- [ ] CI 通过

## 影响范围
- 受影响模块：...
- 是否破坏性变更：否

## 相关
- Closes #123
- ADR: ADR-0004
```

### 4.3 PR 大小
- **小 PR 优于大 PR**
- 一个 PR 应该能在 30 分钟内 review 完
- 超过 500 行变更要拆

### 4.4 自审清单
PR 提交前自查：
- [ ] 编译通过、测试通过
- [ ] 无 `printf` / 调试代码遗留
- [ ] commit 信息符合规范
- [ ] 文档已更新（如有 API 变更）
- [ ] CHANGELOG 已加（如有用户可见变更）
- [ ] 自我 review 一遍 diff

---

## 5. Review 流程

### 5.1 谁 review
- 自己（强制自 review）
- 至少 1 位他人（如有合作者）

### 5.2 Review 关注点

按优先级：
1. **正确性** — 逻辑、边界、并发
2. **安全** — 注入、溢出、敏感信息
3. **可维护** — 命名、抽象、测试
4. **性能** — 大 O、IO、内存
5. **风格** — 命名、格式

### 5.3 Review 反馈级别

| 标记         | 含义                 |
|------------|--------------------|
| `nit:`     | 小建议，可改可不改          |
| `suggest:` | 建议改                |
| `must:`    | 必须改才能合并            |
| `ask:`     | 求解释                |
| `praise:`  | 表扬（鼓励好做法）          |

---

## 6. Merge 策略

### 6.1 默认：Squash and merge
- 把 PR 的多个 commit 压成一个
- main 历史干净
- commit message 用 PR 标题 + body

### 6.2 例外：Rebase
- 重大变更（如版本升级）想保留多 commit 历史时

### 6.3 永远不用：Merge commit
- 主分支不要分叉

---

## 7. Tag 与 Release

### 7.1 版本号（Semver）
```
MAJOR.MINOR.PATCH
1.2.3
```
- MAJOR — 不兼容的 API 变更
- MINOR — 向后兼容的新功能
- PATCH — 向后兼容的 bug 修复

### 7.2 打 tag

```bash
git checkout main && git pull
git tag -a v0.1.0 -m "v0.1.0 - MVP release"
git push origin v0.1.0
```

### 7.3 Release Notes

在 GitHub Releases 页面：
```markdown
## v0.1.0 (2026-05-19)

### Features
- feat(cli): add init/import/list/search/review commands
- feat(parser): markdown extraction for cards

### Fixes
- fix(db): handle WAL recovery

### Docs
- 完整文档体系

完整变更见 [CHANGELOG.md](./CHANGELOG.md)
```

---

## 8. 保护规则（GitHub 设置）

main 分支应配置：
- ✓ Require pull request before merging
- ✓ Require status checks to pass（CI）
- ✓ Require linear history
- ✓ Do not allow bypassing
- ✗ 禁止直接 push

---

## 9. 同步 fork（外部贡献者）

```bash
git remote add upstream https://github.com/<owner>/bagu-cli.git
git fetch upstream
git checkout main
git rebase upstream/main
git push origin main
```

---

## 10. 常用别名

`~/.gitconfig`：
```ini
[alias]
    co = checkout
    br = branch
    st = status -sb
    ci = commit
    cm = commit -m
    last = log -1 HEAD
    unstage = reset HEAD --
    lg = log --oneline --graph --decorate --all
    amend = commit --amend --no-edit
    please = push --force-with-lease
```

---

## 11. 相关文档

- [Conventional Commits 规范](https://www.conventionalcommits.org/)
- [Semantic Versioning](https://semver.org/)
- [编码规范](./coding-standards.md)
