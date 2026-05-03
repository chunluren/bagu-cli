# 用户手册

> 给最终用户的使用指南。开发文档在 [`docs/`](../README.md) 其他目录。

## 目录

| 文档 | 内容 |
|---|---|
| [快速开始](./getting-started.md) | 5 分钟从零到第一次面试 |
| [配置参考](./config.md) | `~/.bagu/config.toml` 字段说明 |
| [Web UI 指南](./web-ui.md) | `bagu serve` / 局域网 / PWA 安装 |
| [LLM 接入](./llm-providers.md) | OpenAI / Claude / Ollama / 兼容代理 |
| [Markdown 格式规范](./markdown-format.md) | 题库文档怎么写才能被识别 |
| [导出题库](./export.md) | 导出到 Anki / REST API |
| [常见问题](./faq.md) | 安装 / 数据 / 复习 / 面试 / 性能 / 隐私 |

## 命令速查

```bash
bagu init                      # 初始化 ~/.bagu/
bagu import <path>             # 导入 markdown
bagu list [topic]              # 列主题 / 章节
bagu show <id>                 # 看卡片
bagu search <kw> [--topic T]   # FTS 搜索
bagu review [--topic T]        # 终端 TUI 复习
bagu interview --topic T -n N  # AI 模拟面试
bagu stats [--heatmap]         # 学习统计
bagu serve [--bind --port]     # Web UI（含前端）
bagu export anki [--topic T]   # 导出 Anki txt
bagu config get/set/list       # 配置管理
bagu --version / --help
```

## 快捷入口

- 第一次用？→ [快速开始](./getting-started.md)
- 想接 Ollama？→ [LLM 接入 / Ollama](./llm-providers.md#ollama本地)
- 手机访问？→ [Web UI / 局域网手机访问](./web-ui.md#局域网--手机访问)
- 题库怎么写？→ [Markdown 格式规范](./markdown-format.md)
- 出问题了？→ [FAQ](./faq.md)
