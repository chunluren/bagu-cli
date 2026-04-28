# 术语表（Glossary）

> **状态**：Living
> **更新**：2026-04-29

---

## A

**ADR (Architecture Decision Record)**
架构决策记录。一个 markdown 文件，描述一个重要技术决策的背景、决定、备选方案与后果。

**Anki**
开源记忆卡片软件，启发了 bagu-cli 的复习算法（SM-2）。

**ASan (AddressSanitizer)**
LLVM/GCC 的内存错误检测器，检测越界、UAF、双重释放等。

---

## B

**bagu（八股）**
中文俗语，原指明清科举的"八股文"。在 IT 圈泛指"面试常考的标准化知识"。本项目名取自此意。

**Buffer Pool**
InnoDB 的内存缓存，存放数据页。本项目数据模型不涉及，但八股文档常考。

---

## C

**Card（卡片）**
本项目的最小复习单元。包含 question + answer + 元数据（topic、tag、难度等）。

**CBO (Cost-Based Optimizer)**
基于代价的查询优化器，MySQL 用的就是这个。

**Chapter（章节）**
markdown 中的二级标题（`## 第 N 章`）。

**CI/CD**
持续集成 / 持续交付。本项目用 GitHub Actions。

**cmark**
CommonMark 规范的 C 语言实现。本项目用它解析 markdown。

**CommonMark**
Markdown 的标准化规范。

**Conventional Commits**
git commit message 规范。本项目采用。

---

## D

**DAO (Data Access Object)**
数据访问对象，封装数据库 CRUD。本项目 db/ 模块的核心。

**DoD (Definition of Done)**
完成定义。明确"什么算做完了"。

---

## E

**e2e (end-to-end test)**
端到端测试。本项目用 bash 脚本调用 `bagu` 命令并验证。

**Ebbinghaus Forgetting Curve（艾宾浩斯遗忘曲线）**
学习遗忘规律。SM-2 算法的理论基础。

---

## F

**FetchContent**
CMake 内置的依赖管理机制，从 git 拉源码并集成。

**FTS5 (Full-Text Search 5)**
SQLite 的全文搜索扩展。本项目搜索引擎基础。

**FTXUI**
现代 C++ 的终端 UI 库（声明式）。本项目用它做 review/interview 界面。

---

## G

**Google Benchmark**
Google 出品的微基准测试库。本项目性能基准用它。

**GoogleTest (gtest)**
Google 出品的 C++ 单元测试框架。

**GTID (Global Transaction Identifier)**
MySQL 的全局事务 ID。八股内容，本项目数据模型无关。

---

## H

**HLD (High-Level Design)**
高层架构设计。

**HPACK**
HTTP/2 的 header 压缩算法。八股相关。

---

## I

**IDE**
集成开发环境。推荐 VS Code / CLion。

**Index（索引）**
数据库索引。本项目自身用了 SQLite 索引。

**InnoDB**
MySQL 的默认存储引擎。八股核心。

---

## J

**jemalloc / tcmalloc / mimalloc**
高性能内存分配器。本项目可链接 jemalloc 提升性能。

---

## L

**LLD (Low-Level Design)**
低层（详细）设计。本项目 modules.md 即此。

**LLM (Large Language Model)**
大语言模型。本项目 interview 命令调用。

**LSM-Tree (Log-Structured Merge Tree)**
LevelDB / RocksDB 用的存储结构。八股相关。

---

## M

**Markdown**
轻量标记语言。本项目八股文档格式。

**MVCC (Multi-Version Concurrency Control)**
多版本并发控制。MySQL 八股核心，本项目数据模型无关。

**MVP (Minimum Viable Product)**
最小可行产品。本项目 v0.1 即 MVP。

---

## P

**PRD (Product Requirements Document)**
产品需求文档。

**Prepared Statement**
SQL 预编译语句。本项目 DAO 必用，避免 SQL 注入。

---

## R

**Reactor 模式**
基于事件多路复用的并发模式（如 epoll）。八股相关。

**Result<T>**
本项目错误处理的核心类型，替代异常。`Result<T>` 要么持有 T，要么持有 Error。

**Review**
本项目的"复习"功能 = 一次卡片提问 + 评分。

**RAII (Resource Acquisition Is Initialization)**
C++ 资源管理范式：构造时获取资源，析构时释放。

---

## S

**Scope**
本项目命名空间和模块边界的概念。`bagu::db::CardDao` 即是。

**Semver (Semantic Versioning)**
语义化版本号 `MAJOR.MINOR.PATCH`。本项目遵循。

**Sink（spdlog 术语）**
日志输出目的地（stdout / file / syslog）。

**SM-2 算法**
SuperMemo 创始人 Piotr Wozniak 设计的间隔重复算法。Anki 默认用。本项目复习核心。

**SSE (Server-Sent Events)**
HTTP 流式响应协议。本项目 LLM 流式输出用。

**SSO (Small String Optimization)**
std::string 的小字符串优化（短字符串内联）。八股。

---

## T

**TLD (Top-Level Domain)**
本项目无关，常出现在网络八股。

**Topic（主题）**
本项目的文档单位 = 一个 markdown 文件 = 一个主题（如 mysql）。

**toml++**
C++ 的 TOML 解析库。本项目读 config 用。

**TUI (Text User Interface)**
终端用户界面。本项目用 FTXUI 实现。

---

## U

**UAF (Use After Free)**
释放后使用，C/C++ 经典 bug。ASan 能检测。

**UBSan (UndefinedBehaviorSanitizer)**
LLVM/GCC 的未定义行为检测器。

---

## W

**WAL (Write-Ahead Logging)**
预写日志。
- SQLite 的 WAL 模式：本项目启用以提升并发。
- MySQL InnoDB 的 redo log 也是 WAL。八股。

---

## 缩略语

| 缩略 | 全称                            | 中文           |
|----|-------------------------------|--------------|
| API | Application Programming Interface | 应用编程接口   |
| BNL | Block Nested Loop Join         | 块嵌套循环连接   |
| CAS | Compare And Swap               | 比较并交换      |
| CRUD | Create / Read / Update / Delete | 增删改查       |
| DML | Data Manipulation Language     | 数据操作语言    |
| DDL | Data Definition Language       | 数据定义语言    |
| FK | Foreign Key                    | 外键          |
| HLD | High-Level Design              | 高层设计        |
| LLD | Low-Level Design               | 详细设计        |
| MVP | Minimum Viable Product         | 最小可行产品    |
| OLAP | Online Analytical Processing   | 联机分析处理    |
| OLTP | Online Transaction Processing  | 联机事务处理    |
| ORM | Object-Relational Mapping       | 对象-关系映射   |
| PK | Primary Key                     | 主键          |
| RAII | Resource Acquisition Is Initialization | 资源获取即初始化 |
| RC | Read Committed                  | 读已提交        |
| RR | Repeatable Read                 | 可重复读        |
| RWlock | Read-Write Lock              | 读写锁         |

---

## 相关

- [PRD](../product/PRD.md)
- [架构总览](../architecture/overview.md)
