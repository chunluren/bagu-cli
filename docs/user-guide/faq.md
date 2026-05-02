# 常见问题（FAQ）

## 安装 / 构建

### Q: cmake 报错找不到 SQLite3 / libcurl / OpenSSL
A: 系统包没装。

```bash
# Ubuntu / Debian
sudo apt install libsqlite3-dev libcurl4-openssl-dev libssl-dev

# Fedora
sudo dnf install sqlite-devel libcurl-devel openssl-devel

# macOS
brew install sqlite curl openssl
```

### Q: 二进制运行时报 "embedded web assets missing"
A: 你的源码构建跳过了 web 那一步。重做：

```bash
cd web && npm ci && npm run build && cd ..
rm -rf build && cmake -B build && cmake --build build -j
```

### Q: build 速度太慢（首次 30+ 分钟）
A: 首次 FetchContent 拉 spdlog / json / ftxui / cpp-httplib / GoogleTest，需要网络。后续 ccache：

```bash
sudo apt install ccache
export CC="ccache gcc" CXX="ccache g++"
cmake -B build && cmake --build build -j
```

## 数据 / 导入

### Q: import 之后数据库在哪？
A: `~/.bagu/bagu.db`（或 `$BAGU_HOME/bagu.db` 如果设了环境变量）。

### Q: 我想重置数据库重新开始
A:
```bash
rm ~/.bagu/bagu.db
bagu import ~/my-docs/
```

### Q: 删除某个主题
A: 当前没有 CLI 命令，直接 SQL：

```bash
sqlite3 ~/.bagu/bagu.db "DELETE FROM topic WHERE name='mysql';"
# CASCADE 会清掉 chapter / card / review / review_history
```

### Q: import 同一文件第二次有没有副作用
A: 默认按 SHA256 跳过；改了内容后想重导用 `--force`，但当前实现会丢复习历史（card.id 重建）。Roadmap 修复中。

## 复习

### Q: 复习卡片的间隔逻辑是？
A: SuperMemo-2 算法：
- 评分 0/1/2（失败）：repetitions 重置，下次复习 1 天后
- 评分 3：interval 用上次的 ease factor 加成
- 评分 4/5：同上 + ease 上调

具体参数 `src/core/sm2.cpp`。

### Q: 跳过的卡片会重新出现吗？
A: 跳过不写 review，下次还在 due 列表。

### Q: 如何 reset 单卡的复习状态
A:
```bash
sqlite3 ~/.bagu/bagu.db "DELETE FROM review WHERE card_id=42;"
sqlite3 ~/.bagu/bagu.db "DELETE FROM review_history WHERE card_id=42;"
```

## AI 面试

### Q: interview 一直转圈不出题
A: 可能：
1. API key 没设：`echo $OPENAI_API_KEY`
2. base_url 不通：`curl -I https://api.openai.com/v1/chat/completions`
3. 代理走了：`unset http_proxy https_proxy`
4. 调试：`SPDLOG_LEVEL=debug bagu interview --topic mysql`

### Q: 评分总是 0/10
A: 模型没在回复里写「评分：N/10」。检查 `src/llm/prompts.cpp` 里的评分 prompt 是不是被覆盖了，或者你的模型不擅长跟随 system prompt。换更强的模型（gpt-4o）通常解决。

### Q: 怎么看历史会话
A:
```bash
sqlite3 ~/.bagu/bagu.db
SELECT id, topic, started_at, total_score, question_count FROM interview_session ORDER BY started_at DESC LIMIT 10;
SELECT question_no, ai_score, substr(question, 1, 50) FROM interview_qa WHERE session_id = 1;
```

或 Web UI 还没做历史会话页（v0.5 计划）。

## Web UI

### Q: bagu serve 启动失败 "listen 失败"
A: 端口被占。换端口：`bagu serve --port 9999`。或杀旧进程：

```bash
lsof -i :8780
kill <PID>
```

### Q: 浏览器打开是空白页
A: 检查：
1. 二进制有没有内嵌 web 资源：`./bagu serve` 启动日志会写 `Embedded Web UI: 12 assets`，0 就是没装
2. 控制台报错？打开 DevTools 看 Network / Console
3. 清浏览器缓存（PWA service worker 可能缓存了旧版）

### Q: 局域网手机访问 401
A: `--bind 0.0.0.0` 启动时如果带了 `--token`，浏览器需要带 `Authorization: Bearer <token>` header。简单浏览器不方便带 header；要么不要 token（仅信任的私网），要么用 SSH tunnel：

```bash
ssh -L 8780:127.0.0.1:8780 user@server
# 本地浏览器开 localhost:8780
```

### Q: PWA 装完打不开 / 一直显示旧版
A: Service Worker 老缓存。方法：
- Chrome：DevTools → Application → Service Workers → Unregister，刷新
- Safari iOS：长按 PWA 图标删除，重新「添加到主屏」
- 改 `web/public/sw.js` 里 `CACHE_VERSION = 'v1'` 为 `'v2'` 重新发布

## 性能

### Q: 二进制 40 MB 是不是太大
A: 大头是嵌入的 React + highlight.js（~20MB）+ SQLite + curl + OpenSSL 静态部分。CLI 单独使用其实 strip 一下能小很多：

```bash
strip build/src/bagu  # ~30 MB
upx --best build/src/bagu  # ~10 MB（启动会慢一点）
```

### Q: 导入 1000+ 文件会卡吗？
A: 实测 4 文件 / 395 cards = 0.16s。1000 文件估计 30s-1min。瓶颈在 markdown 解析，不在 SQLite（SQLite 写入是事务批量的）。

### Q: FTS 中文搜索精度不高
A: 当前用 SQLite 内置 `unicode61` tokenizer，中文按字符切。Roadmap 加 N-gram 改善。

## 隐私

### Q: 数据会上传吗？
A:
- CLI / TUI 路径：完全本地，不联网（除非你跑 `bagu interview`）
- `bagu interview`：把题目和你的答案发给 LLM provider；自托管 Ollama 可避免
- `bagu serve`：默认 `127.0.0.1`，仅本机访问；显式 `--bind 0.0.0.0` 才暴露

### Q: API key 会不会被记日志
A: 只在 LLM 请求时作为 `Authorization` header；日志只记 method/path/status，不记 header。但生产场景仍建议用最小权限的 key。

## 调试

### Q: 我想看 SQL 是怎么执行的
A:
```bash
SPDLOG_LEVEL=debug bagu list
```

### Q: 我想看 LLM 请求/响应原文
A:
```bash
SPDLOG_LEVEL=debug bagu interview --topic mysql 2>&1 | tee debug.log
```

### Q: 报告 bug
A: <https://github.com/chunluren/bagu-cli/issues> — 带上版本（`bagu --version`）和 `SPDLOG_LEVEL=debug` 的输出。
