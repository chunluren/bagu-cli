# Architecture Decision Records (ADR)

> 重要架构决策的记录。每个 ADR 描述一个具体的决策、其上下文与后果。

## 什么是 ADR

参考 Michael Nygard 的 [Architecture Decision Records](https://cognitect.com/blog/2011/11/15/documenting-architecture-decisions)。

**ADR 用于：**
- 重大技术选型
- 设计取舍（trade-off）
- 推翻之前决定时

**ADR 不用于：**
- 实现细节（写代码注释或 LLD 即可）
- 临时变通方案

## 状态流转

```
Proposed → Accepted → (Deprecated → Superseded by ADR-NNNN)
```

## 索引

| 编号 | 标题                               | 状态     | 日期       |
|----|----------------------------------|--------|----------|
| 0001 | [选用 C++20](./0001-use-cpp20.md) | Accepted | 2026-04-29 |
| 0002 | [选 SQLite + FTS5 作为存储](./0002-use-sqlite-with-fts5.md) | Accepted | 2026-04-29 |
| 0003 | [用 CMake FetchContent 管理依赖](./0003-use-cmake-fetchcontent.md) | Accepted | 2026-04-29 |

## 添加新 ADR

1. 复制 [template.md](./template.md)
2. 编号递增（0004、0005...）
3. 文件名：`NNNN-short-title.md`
4. 在本 README 添加索引
5. 提交时 commit 信息：`docs(adr): add ADR-NNNN <title>`
