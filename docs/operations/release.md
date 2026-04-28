# 发布流程

> **状态**：Approved
> **更新**：2026-04-29

---

## 1. 版本号规则

遵循 [Semantic Versioning 2.0](https://semver.org/)：

```
MAJOR.MINOR.PATCH[-PRERELEASE][+BUILD]
```

| 部分        | 何时升                              |
|-----------|----------------------------------|
| MAJOR     | 不兼容的 API 变更                       |
| MINOR     | 向后兼容的新功能                          |
| PATCH     | 向后兼容的 bug 修复                      |
| PRERELEASE| `alpha.1` / `beta.2` / `rc.1`     |

例：`v1.0.0`、`v1.0.1`、`v1.1.0-beta.1`

---

## 2. 发布频率

| 类型           | 频率              | 决策者     |
|------------|-----------------|---------|
| MAJOR      | 半年级           | 项目负责人  |
| MINOR      | 1-2 个月         | 项目负责人  |
| PATCH      | 按需（紧急 bug）   | 任何维护者  |
| Prerelease | 大版本前测试        | 项目负责人  |

---

## 3. 发布前检查清单

### 3.1 代码就绪
- [ ] 所有计划的 issue / PR 已合并
- [ ] CI 在 main 上是绿色
- [ ] 单元测试覆盖率达标
- [ ] e2e 测试通过
- [ ] 性能基准无明显回退

### 3.2 文档就绪
- [ ] CHANGELOG.md 已更新
- [ ] README 中的版本号已更新
- [ ] 新功能有用户文档
- [ ] 破坏性变更有迁移指南

### 3.3 兼容性
- [ ] 数据库 schema 迁移脚本已写
- [ ] 配置文件向后兼容（或有 migrator）
- [ ] CLI 命令向后兼容（或废弃 + 警告）

---

## 4. 发布步骤

### 4.1 准备分支
```bash
# 大版本可选：先开 release branch 稳定一段时间
git checkout -b release/v1.0.0
# bug 修复在此 cherry-pick
```

小版本直接在 main 发：

### 4.2 更新版本号

`include/bagu/version.h`：
```cpp
constexpr const char* VERSION = "1.0.0";
```

`CMakeLists.txt`：
```cmake
project(bagu-cli VERSION 1.0.0 ...)
```

### 4.3 更新 CHANGELOG

`CHANGELOG.md`：
```markdown
## [1.0.0] - 2026-06-30

### Added
- AI 模拟面试功能 (#12)
- 学习热力图 (#15)

### Changed
- 升级 SQLite 到 3.45 (#20)

### Fixed
- 修复 review TUI 在小终端下崩溃 (#18)

### Deprecated
- `bagu legacy-cmd` 将在 v2.0 移除

### Removed
- 移除已废弃的 `--old-flag` (#22)

### Security
- 修复 import 路径遍历漏洞 (#25)
```

### 4.4 提交并打 tag

```bash
git add include/bagu/version.h CMakeLists.txt CHANGELOG.md
git commit -m "chore(release): v1.0.0"
git push

# 打 tag
git tag -a v1.0.0 -m "Release v1.0.0

详见 CHANGELOG.md"

git push origin v1.0.0
```

### 4.5 等 CI 自动发布

`release.yml` 会：
1. 构建多平台二进制（Linux x86/ARM、macOS x86/ARM）
2. 打包 .tar.gz
3. 创建 GitHub Release
4. 上传产物
5. 自动生成 release notes

### 4.6 验证发布

```bash
# 下载并测试
wget https://github.com/.../releases/download/v1.0.0/bagu-linux-x86_64.tar.gz
tar -xzf bagu-linux-x86_64.tar.gz
./bagu --version
# 期望：bagu 1.0.0
```

### 4.7 公告

- GitHub Release 页面（CI 自动建）
- Twitter / X
- Reddit r/cpp
- HackerNews（重大版本）
- 个人博客

---

## 5. PATCH 发布（紧急 bug 修复）

```bash
# 从最新 tag 拉分支
git checkout v1.0.0
git checkout -b hotfix/v1.0.1

# 修复 bug
git commit -m "fix: critical issue"

# 测试
cmake --build build && ctest

# 合并回 main
git checkout main
git merge hotfix/v1.0.1

# 发版
# 改 version.h / CHANGELOG / 打 tag
git tag -a v1.0.1 -m "Hotfix v1.0.1"
git push origin v1.0.1
```

---

## 6. Prerelease（预发布）

测试大版本：
```bash
git tag -a v2.0.0-beta.1 -m "v2.0.0 beta 1"
git push origin v2.0.0-beta.1
```

CI 会标记为 `prerelease=true`，包管理器（如 brew）不自动升级。

---

## 7. 撤回发布（紧急情况）

如果发现严重问题：

### 7.1 GitHub Release
- 可以"标记为 prerelease"或"删除 release"
- ⚠️ tag 一旦推送不要删（破坏其他人的 git 状态）

### 7.2 修补
- 立即发布 PATCH 版本
- 在 CHANGELOG 中说明 v1.0.0 的问题，建议升级

---

## 8. 数据库 schema 迁移

### 8.1 写迁移脚本

`src/db/migrations/0005_add_card_type.sql`：
```sql
ALTER TABLE card ADD COLUMN card_type TEXT DEFAULT 'qa';
UPDATE card SET card_type = 'qa' WHERE card_type IS NULL;
```

### 8.2 自动应用
启动时检测 schema_version，按顺序执行未应用的脚本。

### 8.3 兼容性
- 新版只能向上迁移（不能降级）
- 重大 schema 变更跨大版本（如 v1 → v2）

---

## 9. 配置文件迁移

### 9.1 字段新增
- 加默认值
- 不需要迁移

### 9.2 字段重命名 / 删除
```cpp
// 启动时检查并迁移
void migrate_config_v1_to_v2(toml::table& cfg) {
    if (auto old = cfg["llm"]["api_key"]; old) {
        cfg["llm"]["api_key_env"] = old;
        cfg["llm"].as_table()->erase("api_key");
        spdlog::warn("配置项 llm.api_key 已废弃，请改用 llm.api_key_env");
    }
}
```

---

## 10. 回滚

如果用户升级后出问题：

```bash
# 安装旧版本
wget https://github.com/.../releases/download/v0.9.0/bagu-linux-x86_64.tar.gz
# 替换二进制

# 数据库需要兼容（schema 不要降级）
# 否则需要从备份恢复
cp ~/.bagu/bagu.db.backup ~/.bagu/bagu.db
```

---

## 11. 发布度量

每次发版后跟踪：
- 下载量（GitHub Release stats）
- 新 issue 数量（首周）
- 严重 bug 数（决定是否需要 PATCH）

---

## 12. CHANGELOG 维护

### 12.1 Unreleased section

`CHANGELOG.md` 顶部永远有 Unreleased：

```markdown
# Changelog

## [Unreleased]

### Added
- 新功能 X

### Fixed
- 修了 Y

## [1.0.0] - 2026-06-30
...
```

发版时把 Unreleased 改成版本号 + 日期，再在顶部加新的 Unreleased。

### 12.2 写法
- 用户视角，不写代码细节
- 链接 issue / PR
- 按 Added / Changed / Deprecated / Removed / Fixed / Security 分类

---

## 13. 相关文档

- [Semantic Versioning](https://semver.org/)
- [Keep a Changelog](https://keepachangelog.com/)
- [CI/CD](./ci-cd.md)
- [构建说明](./build.md)
