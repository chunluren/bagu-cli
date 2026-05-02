# LLM 接入

`bagu interview` 支持 4 种 provider：

| provider | 默认 base_url | 默认 model | 备注 |
|---|---|---|---|
| `openai` | `https://api.openai.com/v1` | `gpt-4o-mini` | 标准 OpenAI |
| `openai-compat` | （必填） | （必填） | 任何 OpenAI 兼容 API |
| `claude` | `https://api.anthropic.com/v1` | `claude-sonnet-4-6` | 当前用 OpenAI 兼容路径 |
| `ollama` | `http://localhost:11434/v1` | `llama3.2` | 本地，无需 key |

底层都用同一份 `OpenAIClient`（绝大多数兼容服务都遵循这个协议）。

## OpenAI

```toml
[llm]
provider = "openai"
model = "gpt-4o-mini"
api_key_env = "OPENAI_API_KEY"
```

```bash
export OPENAI_API_KEY=sk-...
bagu interview --topic mysql
```

## Anthropic Claude

如果你用直连 Claude 官方 API，目前的 `OpenAIClient` 路径不直接兼容（Claude 官方 API 不是 OpenAI 兼容的；要用专门的 SDK 路径）。当前的 `claude` provider 适用于：

- 用了 OpenAI 兼容的 Claude 代理（如 OneAPI / NewAPI）
- LiteLLM 之类的中间层

```toml
[llm]
provider = "openai-compat"   # 不是 "claude"
base_url = "https://your-proxy.example.com/v1"
model = "claude-sonnet-4-6"
api_key_env = "PROXY_KEY"
```

> Roadmap：v1.x 会加原生 Anthropic Messages API 实现。

## Ollama（本地）

零成本，无 key。

```bash
# 装 Ollama
curl -fsSL https://ollama.com/install.sh | sh

# 拉模型
ollama pull qwen2.5:7b
ollama pull llama3.2

# 让 Ollama 跑起来（默认端口 11434）
ollama serve &
```

```toml
[llm]
provider = "ollama"
model = "qwen2.5:7b"
# api_key_env 缺省即可
```

```bash
bagu interview --topic mysql
```

## 自建代理 / 第三方 OpenAI 兼容服务

通义千问、Moonshot、DeepSeek、Groq、SiliconCloud 等基本都兼容 OpenAI 协议：

```toml
[llm]
provider = "openai-compat"
base_url = "https://api.deepseek.com/v1"
model = "deepseek-chat"
api_key_env = "DEEPSEEK_API_KEY"
```

```bash
export DEEPSEEK_API_KEY=sk-...
bagu interview --topic mysql
```

## 命令行覆盖

`--provider` / `--model` 永远胜过 `config.toml`：

```bash
# 临时切到 ollama 跑一次
bagu interview --topic mysql --provider ollama --model llama3.2

# 临时换大模型
bagu interview --topic mysql --model gpt-4o
```

## 错误码对照

| 错误码 | 含义 | 排查 |
|---|---|---|
| `5001 kLlmAuthFailed` | HTTP 401 | API key 不对 / 环境变量没设 |
| `5002 kLlmRateLimited` | HTTP 429 | 限流 — 等几秒重试 |
| `5003 kLlmInvalidResponse` | JSON 解析失败 | provider 返回了非标准格式 |
| `5004 kLlmProviderUnknown` | 不支持的 provider | 检查拼写 |
| `5101 kHttpStatusError` | 其他 4xx/5xx | 看错误 detail 字段 |
| `5100 kNetworkError` | curl 失败 | 网络 / DNS / TLS 问题 |

## Web UI 里换 provider

「面试」页 → 高级 → 输入 provider 和 model 覆盖。

> ⚠️ Web UI 不支持现场输入 API key —— key 必须在 `bagu serve` 启动前设到环境变量。
