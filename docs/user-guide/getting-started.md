# 快速开始

> 5 分钟从零到第一次 AI 模拟面试。

## 它是什么

**bagu-cli** 是一个本地八股复习工具：

- 把 Markdown 文档（标题 / 章节 / Q&A）当作面试题库
- SQLite + FTS5 全文检索
- SM-2 间隔重复算法做复习排程
- TUI 复习界面（终端里直接用）
- 内嵌 Web UI（`bagu serve` 单进程含前端，可手机访问）
- 接 LLM（OpenAI / Claude / Ollama）做模拟面试 + 自动评分

数据完全在本地（`~/.bagu/bagu.db`），不上传任何信息。

## 安装

### 选 1：Homebrew（推荐）

```bash
brew tap chunluren/bagu
brew install bagu-cli
```

支持 macOS Apple Silicon + Linux x86_64。其他平台 brew 会从源码构建（要 Node 20+ / cmake / sqlite / libcurl / openssl）。

### 选 2：下载预编译包

```bash
# Linux x86_64
curl -LO https://github.com/chunluren/bagu-cli/releases/latest/download/bagu-v0.4.0-linux-x86_64.tar.gz
tar -xzf bagu-v0.4.0-linux-x86_64.tar.gz
sudo mv bagu /usr/local/bin/
```

macOS Apple Silicon 用 `macos-arm64`。Intel Mac 暂未发预编译，请用源码构建（选 3）。

### 选 3：从源码构建

需要：CMake 3.20+ / GCC 11+ 或 Clang 14+ / Node 20+ / SQLite3 / libcurl / OpenSSL。

```bash
git clone https://github.com/chunluren/bagu-cli.git
cd bagu-cli

# 1. 构建前端（产物 web/dist 会被 cmake 嵌入二进制）
cd web && npm ci && npm run build && cd ..

# 2. 构建 C++
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j

# 安装（可选）
sudo cp build/src/bagu /usr/local/bin/
```

构建完的 `bagu` 是单文件二进制（约 40 MB，含前端 + SQLite + curl）。

## 第一次使用

### 1. 初始化数据目录

```bash
bagu init
```

创建 `~/.bagu/`：
- `bagu.db` — SQLite 数据库（首次 import 时创建）
- `config.toml` — 配置（LLM provider / api key 环境变量名）
- `logs/` — 日志

### 2. 准备题库（Markdown）

任何 Markdown 文件，按以下格式：

```markdown
# MySQL 八股文档

## 第一章 索引

### 1.1 B+ 树为什么适合做数据库索引？

**Q1. B+ 树为什么适合做数据库索引？**
B+ 树的非叶子节点只存索引、叶子节点存数据，
单节点能容纳更多 key，树的高度低、IO 次数少。
叶子节点用链表串起来，范围查询直接顺序扫描。

**Q2. B+ 树和 B 树的区别？**
1. B+ 树非叶子节点不存数据，只做索引
2. B+ 树叶子节点链表相连，支持范围查询
...
```

支持的标题层级：
- `# 标题` → 文档标题（topic）
- `## 第 N 章 ...` → 顶级章节
- `### N.M ...` → 子章节
- `**Q数字. ?** ...` 或 `**Q: ?** ...` → 卡片

代码块（```` ``` ````）内不会被识别为问答。

### 3. 导入

```bash
bagu import ~/my-docs/mysql.md
# 或导入整个目录
bagu import ~/my-docs/
```

输出例：
```
Importing from ~/my-docs/...
Found 3 markdown file(s).
  [1/3] mysql.md  → mysql (new, 86 cards)
  [2/3] redis.md  → redis (new, 144 cards)
  [3/3] cpp.md    → cpp (new, 165 cards)
Done in 0.16s. Succeeded: 3, Failed: 0, Total cards: 395
```

文件用 SHA256 判重，重复 import 同一文件会跳过。

### 4. 浏览

```bash
bagu list                 # 列所有主题 + 卡片数
bagu list mysql           # 看 mysql 主题的章节树
bagu show 42              # 看卡片 42 的完整内容
bagu search 'mvcc'        # 全文搜索
bagu search '间隔' --topic mysql -n 5
```

### 5. 复习（终端 TUI）

```bash
bagu review               # 所有到期卡片
bagu review --topic mysql # 指定主题
bagu review --new-only    # 只学新卡
bagu review -n 20         # 限制本次最多 20 张
```

TUI 里：
- `SPACE` 揭示答案
- `0-5` 评分（≥3 算答对，0/1/2 算失败）
- `s` 跳过
- `q` 退出
- `?` 帮助

### 6. Web UI（推荐 — 手机也能用）

```bash
bagu serve              # 默认 127.0.0.1:8780
```

浏览器打开 <http://127.0.0.1:8780>。

想在手机访问：

```bash
bagu serve --bind 0.0.0.0 --token mysecret
```

然后手机连同一 WiFi，访问 `http://<电脑IP>:8780`，浏览器会要求 Bearer token（HTTP header `Authorization: Bearer mysecret`）。

> Web UI 支持安装为 PWA：Chrome / Safari 的 "添加到桌面"。装完图标在主屏，体验接近原生 App。

### 7. AI 模拟面试

先在 `~/.bagu/config.toml` 里配 LLM：

```toml
[llm]
provider = "openai"
model = "gpt-4o-mini"
base_url = "https://api.openai.com/v1"   # 或自定义代理
api_key_env = "OPENAI_API_KEY"            # 从这个环境变量读 key
```

然后：

```bash
export OPENAI_API_KEY=sk-...
bagu interview --topic mysql -n 5
```

或 Web UI 的「面试」页（流式 typewriter 效果）。

## 一份典型的日常工作流

```bash
# 早上
bagu review --topic redis    # 复习昨天的卡片
bagu interview --topic mysql -n 3   # 顺便面试 3 题热身

# 看进度
bagu stats                   # 总览
bagu stats --heatmap         # GitHub 风格热力图
```

或者一直挂 Web UI：

```bash
bagu serve  # 后台跑
```

桌面 / 手机都打开 `http://localhost:8780/review`，随时复习。

## 下一步

- [配置参考](./config.md) — `config.toml` 全字段说明
- [Web UI 指南](./web-ui.md) — PWA 安装 / 手机适配 / token 安全
- [LLM 接入](./llm-providers.md) — OpenAI / Claude / Ollama / 自建代理
- [Markdown 格式规范](./markdown-format.md) — Q&A 编写最佳实践
- [常见问题](./faq.md)
