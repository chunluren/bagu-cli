# CLI 命令规范

> **状态**：Approved
> **版本**：v1.0
> **更新**：2026-04-29

---

## 1. 总体约定

### 1.1 命令格式

```
bagu [GLOBAL OPTIONS] <COMMAND> [SUB-COMMAND] [OPTIONS] [ARGS]
```

### 1.2 全局选项

| 选项                | 说明                            |
|------------------|-------------------------------|
| `-h, --help`     | 显示帮助                         |
| `-v, --version`  | 显示版本号                       |
| `--verbose`      | 详细日志                         |
| `--quiet`        | 静默模式（仅错误）                  |
| `--config <path>`| 自定义配置文件路径                 |
| `--no-color`     | 关闭彩色输出                     |
| `--json`         | 以 JSON 格式输出（脚本友好）        |

### 1.3 退出码

| 码   | 含义                   |
|----|----------------------|
| 0  | 成功                   |
| 1  | 通用错误                |
| 2  | 参数错误                |
| 3  | 配置错误                |
| 4  | 数据库错误              |
| 5  | 网络 / LLM API 错误    |
| 6  | 用户取消（如复习中按 Ctrl+C）|
| 7  | 文件系统错误             |
| 8  | 解析错误                |

### 1.4 输出风格

- **正常输出** → stdout
- **错误信息** → stderr
- **进度信息** → stderr（可被 `2>/dev/null` 隐藏）
- 颜色：默认开，TTY 检测；非 TTY 自动关
- 国际化：错误信息英文，提示文字简体中文

---

## 2. 命令详细规范

### 2.1 `bagu init`

初始化数据目录与配置文件。

**用法：**
```bash
bagu init [--force]
```

**选项：**
| 选项        | 说明                            |
|-----------|-------------------------------|
| `--force` | 已存在则覆盖                      |

**行为：**
1. 创建 `~/.bagu/` 目录（不存在则创建）
2. 写入默认 `~/.bagu/config.toml`
3. 创建 `~/.bagu/bagu.db` 并执行 schema migrations
4. 输出初始化结果

**输出示例：**
```
✓ Created data dir: /home/user/.bagu
✓ Wrote config:     /home/user/.bagu/config.toml
✓ Initialized db:   /home/user/.bagu/bagu.db (v1)

Next:
  bagu import <path-to-markdown-docs>
```

**退出码：** 0 / 3 / 4 / 7

---

### 2.2 `bagu import <path>`

导入 markdown 八股文档。

**用法：**
```bash
bagu import <path> [--force] [--topic-name <name>]
```

**参数：**
| 参数      | 说明                                |
|---------|-----------------------------------|
| `path`  | md 文件或目录（递归）                  |

**选项：**
| 选项                  | 说明                                  |
|---------------------|-------------------------------------|
| `--force`           | 即使内容未变也重新解析                    |
| `--topic-name <name>` | 指定 topic name（默认从文件名推导）        |

**行为：**
1. 扫描路径下所有 `.md` 文件
2. 对每个文件计算 SHA256，与 db 中已有 hash 比对
3. 不变 → 跳过；变了或新文件 → 解析并入库
4. 用事务保证原子性

**输出示例：**
```
Scanning /home/ly/bagu-docs/...
Found 3 markdown files.

  [1/3] mysql-interview.md       → mysql      | 87 cards (new)
  [2/3] redis-interview.md       → redis      | 87 cards (unchanged, skip)
  [3/3] cpp-network-interview.md → cpp-net    | 85 cards (updated, +12)

Done in 2.3s. Total: 259 cards.
```

**退出码：** 0 / 2 / 4 / 7 / 8

---

### 2.3 `bagu list [topic]`

列出主题与章节。

**用法：**
```bash
bagu list [topic] [--tree] [--with-progress]
```

**参数：**
| 参数      | 说明                            |
|---------|-------------------------------|
| topic   | 主题名（可选，省略则列所有主题）       |

**选项：**
| 选项                | 说明                          |
|------------------|-----------------------------|
| `--tree`         | 树形展示                       |
| `--with-progress`| 显示每章已学进度                 |

**输出示例：**
```bash
$ bagu list
TOPIC      CARDS  CHAPTERS  PROGRESS
mysql        87        10    45%
redis        87        11    30%
cpp-net      85        12     0%

$ bagu list mysql --tree
mysql (87 cards, 45% mastered)
├── 第 1 章 架构与基础       (10 cards, 70%)
├── 第 2 章 存储引擎          (10 cards, 60%)
├── 第 3 章 索引              (20 cards, 40%)
...
```

**退出码：** 0 / 2 / 4

---

### 2.4 `bagu search <keyword>`

全文搜索卡片。

**用法：**
```bash
bagu search <keyword> [--topic <topic>] [--limit <n>] [--json]
```

**参数：**
| 参数      | 说明                            |
|---------|-------------------------------|
| keyword | 搜索关键词                       |

**选项：**
| 选项               | 说明                          |
|----------------|-----------------------------|
| `--topic <topic>` | 限定主题                       |
| `--limit <n>`     | 最大返回数（默认 20）            |
| `--json`          | JSON 输出                     |

**输出示例：**
```
$ bagu search "MVCC"
Found 3 cards:

[mysql/4.4]  MVCC 三大组件
  隐藏字段、undo log 版本链、Read View ...

[mysql/Q43]  MVCC 怎么实现？
  三要素：隐藏字段（trx_id / roll_ptr）+ undo 版本链 + Read View ...

[mysql/Q44]  Read View 可见性规则？
  trx_id == creator：可见；< up_limit：可见 ...

(showing 3 of 3, query took 12ms)
```

**退出码：** 0 / 2 / 4

---

### 2.5 `bagu review`

进入交互式复习模式（TUI）。

**用法：**
```bash
bagu review [--topic <topic>] [--num <n>] [--new-only] [--all]
```

**选项：**
| 选项               | 说明                                |
|----------------|-----------------------------------|
| `--topic <topic>` | 限定主题                              |
| `--num <n>`       | 复习题数（默认按配置 daily_target）    |
| `--new-only`      | 只学新卡片                           |
| `--all`           | 复习所有到期卡片（不限制数量）           |
| `--cram`          | 突击模式（不更新 SM-2 状态）            |

**TUI 操作：**
| 按键    | 功能                       |
|-------|--------------------------|
| Space | 显示答案                   |
| 1-5   | 评分（评分后自动下一题）        |
| j / k | 翻页（查看 markdown 长答案）  |
| s     | 跳过本题                   |
| u     | 撤销上次评分                |
| q     | 退出                       |
| ?     | 显示帮助                   |

**退出码：** 0 / 4 / 6

---

### 2.6 `bagu interview`

AI 模拟面试。

**用法：**
```bash
bagu interview --topic <topic> [--num <n>] [--difficulty <level>]
```

**选项：**
| 选项                  | 说明                                  |
|--------------------|-------------------------------------|
| `--topic <topic>`  | 主题（必需）                            |
| `--num <n>`        | 问题数（默认 5）                        |
| `--difficulty <lv>`| easy / medium / hard / adaptive（默认） |
| `--provider <p>`   | 覆盖配置中的 LLM 提供商                  |

**交互流程：**
```
[AI 面试官] 我们开始第 1 题（MySQL · 中等）：
            请简要介绍 MVCC 的实现原理。

> 请输入答案（多行 Ctrl+D 结束）:
你：MVCC 是 InnoDB 的多版本并发控制...
   (Ctrl+D)

[AI 面试官] 评分：7/10

✓ 抓住了核心：隐藏字段 + undo + Read View
✗ 没说清 RC 与 RR 的 Read View 时机差异
✗ 缺少快照读 vs 当前读的对比

下一题难度：中等偏难

第 2 题：...
```

**结束后：**
```
========== 面试总结 ==========
总分：7.2/10
答题：5 道
耗时：18 分钟

强项：MVCC 原理、索引设计
弱项：锁机制深度、主从复制细节

会话已保存到 db（id=42），可用 bagu interview --resume 42 复盘
```

**退出码：** 0 / 5 / 6

---

### 2.7 `bagu stats`

学习统计。

**用法：**
```bash
bagu stats [--topic <topic>] [--heatmap] [--json] [--days <n>]
```

**选项：**
| 选项                | 说明                          |
|----------------|-----------------------------|
| `--topic <topic>` | 限定主题                       |
| `--heatmap`       | 显示热力图（90 天）             |
| `--days <n>`      | 时间窗口（默认 90 天）          |
| `--json`          | JSON 输出                     |

**输出示例：**
```
$ bagu stats
========== 总览 ==========
连续打卡：    7 天
总学习时间：  12.5 小时
今日复习：    18 / 20

========== 各主题 ==========
TOPIC      已学/总  正确率  下次到期
mysql      40/87    78%      3 张
redis      30/87    82%      0 张
cpp-net     0/85     -      85 张（未学）

========== 最薄弱（最近答错最多）==========
1. [mysql/Q49] 主从复制流程？             错 3 次
2. [redis/Q24] Set 和 ZSet 区别？         错 2 次
3. [mysql/Q57] 死锁分析                   错 2 次

$ bagu stats --heatmap
        Apr  May  Jun
Mon     ▁ ▃ ▆ █ ▇ ▅ ▃ ▁ ▂ ▄ █ ▆ ▃
Tue     ▂ ▄ █ ▆ ▃ ▁ ▂ ▄ █ ▇ ▆ ▄ ▂
...
图例：▁=1   ▂=2-4   ▃=5-9   ▄=10-19   ▅=20-39   ▆=40-79   ▇=80-159   █=160+
```

**退出码：** 0 / 4

---

### 2.8 `bagu config`

配置管理。

**用法：**
```bash
bagu config get <key>
bagu config set <key> <value>
bagu config list
bagu config reset
```

**示例：**
```bash
$ bagu config set llm.provider openai
$ bagu config get review.daily_target
20
$ bagu config list
[general]
data_dir = ~/.bagu
docs_dir = ~/workspaces/bagu-docs
default_topic = mysql

[review]
daily_target = 20
new_cards_per_day = 5
...
```

**退出码：** 0 / 3

---

### 2.9 `bagu show <card-id>`

查看单张卡片完整内容。

**用法：**
```bash
bagu show <card-id>
```

**输出示例：**
```
========== Card #42 ==========
Topic:      mysql
Chapter:    第 4 章 事务与 MVCC
Difficulty: 中
Tags:       [mvcc, transaction]

Q: MVCC 怎么实现？

A: 三要素：
  1. 隐藏字段（DB_TRX_ID / DB_ROLL_PTR / DB_ROW_ID）
  2. undo log 版本链
  3. Read View 可见性判断
  ...

复习状态：
  上次复习：2026-04-25  评分 4
  下次到期：2026-05-02  间隔 7 天
  累计复习：5 次   答对率：80%
```

---

### 2.10 `bagu version` / `bagu help`

标准版本与帮助命令，由 CLI11 自动生成。

---

## 3. 命令矩阵

| 命令       | 是否需要 db | 是否联网 | 是否进入 TUI |
|----------|----------|--------|----------|
| init     | ✓ (创建)  | ✗      | ✗        |
| import   | ✓        | ✗      | ✗        |
| list     | ✓        | ✗      | ✗        |
| search   | ✓        | ✗      | ✗        |
| review   | ✓        | ✗      | ✓        |
| interview| ✓        | ✓      | ✓        |
| stats    | ✓        | ✗      | ✗        |
| config   | ✗        | ✗      | ✗        |
| show     | ✓        | ✗      | ✗        |
| version  | ✗        | ✗      | ✗        |

---

## 4. 错误信息约定

### 4.1 格式

```
Error: <短描述> (<错误码>)

  详细原因 / 上下文

提示：可能的解决方案
```

### 4.2 示例

```
Error: Database not found (E4001)

  ~/.bagu/bagu.db 不存在

提示：请先运行 `bagu init` 初始化
```

### 4.3 错误码体系

详见 [错误处理](../operations/error-handling.md)。

---

## 5. Shell 补全

通过 CLI11 自动生成 bash/zsh/fish 补全脚本：

```bash
bagu --completion bash > /etc/bash_completion.d/bagu
```

---

## 6. 相关文档

- [架构总览](./overview.md)
- [错误处理](../operations/error-handling.md)
- [日志规范](../operations/logging.md)
