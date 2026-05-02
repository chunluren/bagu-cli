# Markdown 格式规范

`bagu import` 用的是手写的 Markdown 解析器（不是 cmark），按以下严格规则抽题。

## 文件 → topic

文件名 → topic 名（自动归一化）：

| 文件名 | topic |
|---|---|
| `mysql.md` | `mysql` |
| `mysql-interview.md` | `mysql`（去 `-interview` 后缀） |
| `MySQL八股文档.md` | `mysql`（去「八股文档」后缀） |
| `cpp-network.md` | `cpp-network` |

`bagu list` 列出的是 topic 名。

## 文档结构

```markdown
# MySQL 八股文档（高级版）

## 第一章 索引

### 1.1 B+ 树为什么适合做数据库索引？

**Q1. B+ 树为什么适合做数据库索引？**
B+ 树的非叶子节点只存索引...

**Q2. B+ 树和 B 树的区别？**
1. 非叶子节点不存数据
2. ...

### 1.2 联合索引

**Q1. 联合索引的最左前缀原则是什么？**
...

## 第二章 事务

### 2.1 ACID

**Q1. ACID 分别是什么？**
...
```

## 标题层级

| 标记 | 含义 | 数据库表 |
|---|---|---|
| `# 标题` | 文档标题 → topic.title | `topic` |
| `## 第 N 章 ...` | 顶级章节 | `chapter` (parent_id=NULL) |
| `### N.M ...` | 子章节 | `chapter` (parent_id=父章节) |
| `### N.M ...` （无 Q&A） | 同时建一张 section 卡 | `card` (card_type='section') |
| `**Q数字. ?** ...` | Q&A 卡片 | `card` (card_type='qa') |
| `**Q: ?** ...` | Q&A 卡片（冒号格式） | `card` (card_type='qa') |

## Q&A 格式细节

### 编号格式（推荐）
```markdown
**Q1. 题目？**
答案第一行
答案第二行

**Q2. 下一题？**
...
```

### 冒号格式
```markdown
**Q: 题目？**
答案

**Q: 下一题？**
...
```

### 答案边界

下一个 `**Q...** ` 出现 → 当前 Q&A 答案结束。
或下一个 `### ` / `## ` 出现 → 同上。
或 EOF。

### 代码块内不抽

```markdown
**Q1. printf 的返回值？**
打印的字符数。

​```c
// 注意这里写 Q 的样子也不会被识别
**Q. 这不会被抽**
​```
```

代码块（` ``` ` 包围）内部完全跳过，可以放任何 markdown。

## 实际示例

参考 [bagu-docs](https://github.com/chunluren/bagu-docs) 仓库（私有；格式见已 import 的本地版本）。

## 常见坑

### Q: 我的 Q&A 没有被识别
A: 检查：
- 必须用 `**Q...** ` 加粗格式（不是 `Q...`）
- 数字编号后必须有 `.` 或 `:`（`**Q1 题目**` 不行）
- 加粗结束的 `**` 后必须紧跟空格 + 答案

### Q: 章节号跳号会怎样？
A: 不强制连续。`1.1 → 1.3` 解析器照样读，但 `bagu list` 时显示是按入库顺序。

### Q: 一个文件能有多个 `# 标题` 吗？
A: 可以，但只有第一个 `#` 用作 topic.title；后续的 `#` 被忽略（不会拆 topic）。一个文件 = 一个 topic。

### Q: 重新 import 同一文件会怎样？
A: SHA256 比对 — 内容没变就跳过。改了就触发：
- 旧的 chapter / card 被删
- 新内容重新插入
- 复习历史 (`review` / `review_history`) **保留**（按 card.id 关联，但 card.id 会变 → 实际会丢历史）

> 改文档 → 用 `--force` 强制：
> ```bash
> bagu import mysql.md --force
> ```

## Roadmap

- [ ] **稳定 ID**：用 question 文本的 hash 当 card.id，复习历史跨重新导入保留
- [ ] **图片 / 公式**：当前完全靠 marked + KaTeX 浏览器渲染，导入层不处理
- [ ] **题目标签**：`#tag` 抽到 card.tags
