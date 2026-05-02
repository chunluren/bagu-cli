# Web UI 指南

> `bagu serve` — 单进程含前端，可手机访问，可装为 PWA

## 启动

```bash
bagu serve                              # 默认 127.0.0.1:8780
bagu serve --port 9000                   # 自定义端口
bagu serve --bind 0.0.0.0                # 监听所有网卡（局域网可访问）
bagu serve --bind 0.0.0.0 --token foo    # 同上 + Bearer 鉴权
bagu serve --dev                         # 开发模式（放宽 CORS）
```

`SIGINT` / `SIGTERM` 优雅退出。

## 路由一览

| 路径 | 功能 |
|---|---|
| `/` | 首页 — 主题列表 + 概览统计 |
| `/topics/:name` | 主题详情 — 章节树 + 卡片 |
| `/cards/:id` | 单张卡片完整内容（marked 渲染） |
| `/search?q=` | 全文搜索（关键词高亮） |
| `/review` | SM-2 复习会话（SPACE 揭示 / 0-5 评分） |
| `/interview` | AI 模拟面试（流式 SSE） |
| `/stats` | 学习统计 + GitHub 风格热力图 |

## REST API

完整规范见 [docs/architecture/http-api-spec.md](../architecture/http-api-spec.md)。常用：

```bash
curl http://127.0.0.1:8780/api/health
curl http://127.0.0.1:8780/api/topics
curl http://127.0.0.1:8780/api/cards/42
curl 'http://127.0.0.1:8780/api/search?q=MVCC&topic=mysql&limit=10'
curl http://127.0.0.1:8780/api/stats/overall
curl 'http://127.0.0.1:8780/api/stats/heatmap?days=90'

# 复习
curl 'http://127.0.0.1:8780/api/review/due?topic=mysql&max_new=3'
curl -X POST http://127.0.0.1:8780/api/review/42/grade \
    -H 'Content-Type: application/json' \
    -d '{"score":4,"duration_ms":3000}'

# 面试（SSE 流）
curl -X POST http://127.0.0.1:8780/api/interview/sessions \
    -H 'Content-Type: application/json' \
    -d '{"topic":"mysql","question_count":3}'
# → {"session_id":1,...}
curl -N http://127.0.0.1:8780/api/interview/sessions/1/question
# → data: {"type":"chunk","text":"..."}\n\n
# → data: {"type":"done","content":"..."}\n\n
```

## 局域网 / 手机访问

### 1. 启动绑定到全网卡

```bash
# 必须用 token；裸开 0.0.0.0 是危险的
bagu serve --bind 0.0.0.0 --token mysecret
```

### 2. 看本机 IP

```bash
# Linux / macOS
ip addr | grep 'inet ' | grep -v 127.0.0.1
ifconfig | grep 'inet '
```

### 3. 手机浏览器

`http://<电脑IP>:8780` — 浏览器会先报 401。

### 4. 带 token 访问

简单场景：把 token 拼在 URL 里（**仅原型**，会进 history / log）：

```
http://<IP>:8780/?token=mysecret    # ⚠️ 服务端不解析这个，要改前端
```

正式场景：用 Header。最简便的方法是装 **Modify Headers** 类的浏览器扩展，或者用 ssh tunnel：

```bash
# 在手机端不便，更推荐用 SSH 隧道（电脑↔电脑访问）
ssh -L 8780:127.0.0.1:8780 user@server
# 然后本机浏览器开 http://localhost:8780（不绑外网更安全）
```

### 5. 防火墙

```bash
# Linux UFW
sudo ufw allow 8780/tcp
```

## 安装为 PWA（桌面 / 手机）

`bagu` 自带 manifest + service worker，支持「添加到主屏」：

### Chrome / Edge（桌面）
1. 访问 `http://localhost:8780`
2. 地址栏右侧出现「安装」图标，点击
3. 装完后从开始菜单 / Launchpad 启动，独立窗口、无地址栏

### Safari（iOS）
1. 浏览器打开页面
2. 分享 → 添加到主屏幕
3. 主屏出现 bagu 图标

### Chrome（Android）
1. 三点菜单 → 安装应用
2. 主屏图标启动

PWA 离线策略：
- 静态资源（HTML/JS/CSS/图标）：cache-first
- `/api/*`：永远走网络（离线时直接报错）
- 导航请求离线时回退到首页 shell

## 主题（亮 / 暗 / 跟随系统）

顶部导航右侧 3 段开关：太阳 / 月亮 / 显示器。选择持久化到 `localStorage`（每个浏览器一份）。

## 常见问题

### Q: SSE 流式响应没有逐字显示，最后一次性吐出
A: 中间可能有反向代理（nginx 默认会缓冲）。在 nginx config 里加：
```
proxy_buffering off;
proxy_cache off;
```

### Q: 装成 PWA 后 sw.js 缓存了旧版怎么办
A: `bagu serve` 重启会带新版的资源 hash。但 PWA 的 sw 升级有 1-2 次刷新延迟；强制：DevTools → Application → Service Workers → Unregister，然后刷新。

### Q: `/api/interview/*` 报 404 "session 不在内存"
A: 正常 — 进程内缓存的 session 在 `bagu serve` 重启后丢失。用 Web UI 重新「开始面试」即可（数据库里的历史 qa 不丢）。

### Q: 手机访问首页空白
A: 9 成是 PWA service worker 缓存了旧版。手机浏览器清缓存或重装 PWA。
