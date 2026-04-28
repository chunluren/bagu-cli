# ADR-0002: 选 SQLite + FTS5 作为存储

- **状态**: Accepted
- **日期**: 2026-04-29
- **作者**: Li Yan

## 背景

bagu-cli 需要存储：
- 卡片元数据（题目、答案、tag、章节）
- 复习状态（SM-2 算法所需）
- 复习历史
- 面试会话

数据规模：单用户 < 100 MB，约几千 cards、几万 history。

需求：
- 全文搜索（中英文）
- ACID 事务
- 单文件、易备份
- 跨平台
- 不引入外部服务

## 决策

**使用 SQLite 3 + FTS5（全文搜索扩展）。**

## 备选方案

| 方案           | 优点                          | 缺点                                | 不选理由                       |
|--------------|------------------------------|-----------------------------------|----------------------------|
| **SQLite + FTS5** | 单文件、SQL 完整、内置全文搜索、广泛支持 | 写并发差（单写者）                      | —                          |
| LevelDB / RocksDB | KV 性能高                       | 无 SQL、无全文搜索、需要自己实现关系       | 用不到那么强的写性能              |
| 本地文件 + grep   | 零依赖                          | 复杂查询困难、无事务                     | 不能满足复习状态管理               |
| 嵌入式 LMDB     | 性能高                          | KV、无全文                            | 同 LevelDB                 |
| PostgreSQL     | 功能强大                        | 需要外部服务、用户安装麻烦                 | 单机工具不该依赖服务               |
| DuckDB        | OLAP 强                       | 不适合 OLTP 风格的频繁更新                | 场景不匹配                    |

## 后果

### 正面

- **零依赖**：SQLite 本身可以静态链接
- **数据库就是单文件**，备份只需 `cp`
- **FTS5 支持中文**（用 unicode61 tokenizer 或 N-gram）
- 标准 SQL，易调试（用 `sqlite3` CLI）
- 大量参考资料

### 负面

- 写并发受限（单写者）—— 个人 CLI 工具无影响
- FTS5 中文需要分词配置
- 大数据量（百万级）下不如专业 DB —— 用不到这么大

### 中性

- 需要写迁移脚本管理 schema 演进
- 需要 SQL 知识（求职项目反而是加分）

## 实施

- 数据库文件：`~/.bagu/bagu.db`
- 通过 `find_package(SQLite3 REQUIRED)` 链接
- DAO 层封装在 `src/db/`
- 详细 schema 见 [data-model.md](../data-model.md)

## 相关链接

- [SQLite FTS5 文档](https://www.sqlite.org/fts5.html)
- [data-model.md](../data-model.md)
