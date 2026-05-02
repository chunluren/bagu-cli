# 配置参考

`~/.bagu/config.toml` 全字段说明。`bagu init` 会写一份默认值。

## 完整示例

```toml
# ===== LLM 配置（bagu interview 用） =====
[llm]
# 提供商：openai / openai-compat / claude / ollama
provider = "openai"

# 模型 ID（按 provider 不同）
# - openai: gpt-4o-mini, gpt-4o, gpt-4-turbo, ...
# - claude: claude-sonnet-4-6, claude-opus-4-7, ...
# - ollama: llama3.2, qwen2.5, ...
model = "gpt-4o-mini"

# API 端点（可选；用第三方代理 / 自建服务时改）
# 默认：
#   openai → https://api.openai.com/v1
#   claude → https://api.anthropic.com/v1
#   ollama → http://localhost:11434/v1
base_url = "https://api.openai.com/v1"

# API key 从哪个环境变量读（程序不直接存 key 在 config）
api_key_env = "OPENAI_API_KEY"

# ===== Review（SM-2 复习算法）参数 =====
[review]
# 单次会话默认上限
max_due = 20            # 到期卡片
max_new = 5             # 新卡片
```

## 字段细节

### `[llm]`

| 字段 | 类型 | 默认 | 说明 |
|---|---|---|---|
| `provider` | string | `"openai"` | 见上 |
| `model` | string | `"gpt-4o-mini"` | 模型 ID |
| `base_url` | string | provider 默认 | 自定义 endpoint |
| `api_key_env` | string | `"OPENAI_API_KEY"` | 从环境变量读 key（不直接存 key） |

**为什么用环境变量？** 避免把 key 写到磁盘 / 提交到版本控制。
`config.toml` 可以放公网（仓库示例），但 key 不会出去。

**Ollama 不需要 key**：`api_key_env` 可缺省。

### `[review]` （未实现，规划）

预留区，用于将来调 SM-2 参数（ease floor、initial interval 等）。

## 多 profile（不支持，但有 workaround）

如果你想切换不同的 LLM 配置（如本地 Ollama vs 云端 GPT），用环境变量覆盖：

```bash
# 跑 GPT
export OPENAI_API_KEY=sk-... && bagu interview --topic mysql

# 跑本地 Ollama，绕过 toml
bagu interview --topic mysql --provider ollama --model qwen2.5
```

`--provider` / `--model` 命令行参数永远覆盖 `config.toml`。

## BAGU_HOME 环境变量

把数据目录改到非默认位置：

```bash
export BAGU_HOME=/data/bagu
bagu init    # 在 /data/bagu 下创建
bagu import ...
```

适合：
- 多用户隔离（每人一个 BAGU_HOME）
- 跑测试时不污染真实数据（CI 里就是这么做）
- 把数据放外接 SSD

## 数据库直接操作

```bash
sqlite3 ~/.bagu/bagu.db
.tables
SELECT * FROM topic;
SELECT card_id, repetitions, ease_factor, next_review FROM review;
```

完整 schema 参考 `src/db/migrations.cpp`。

## 日志

`~/.bagu/logs/` 下按日切。默认 INFO 级；调试可设：

```bash
SPDLOG_LEVEL=debug bagu serve
```
