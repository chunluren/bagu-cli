# Web UI 实施路线图

> **状态**：Draft
> **版本**：v1.0
> **创建**：2026-04-30
> **上游**：[roadmap.md](./roadmap.md) / [PRD-web-ui.md](../product/PRD-web-ui.md)

---

## 总体节奏

```
v0.3.0 (10 天)   v0.4.0 (10 天)
  MVP 主流程        体验完善 + AI 流式
  ↓                  ↓
后端 HTTP + REST   /api/interview SSE
前端 4 个页面       /api/stats + 热力图
能跑通             移动端适配 + PWA
```

---

## 里程碑

### M-Web-1（v0.3.0，5/1 - 5/10）— MVP

**主题：** 让 Web UI 跑起来 + 自己用得上。

#### Sprint 1：后端基础（Day 1-3）

- [ ] D1 加 cpp-httplib（FetchContent）+ `bagu serve` 命令骨架
- [ ] D1 路由层：`Server::register_routes` + `--port / --bind / --token` 选项
- [ ] D2 健康检查 `/api/health` + `/api/version` + access log
- [ ] D2 JSON 序列化 helper（model ↔ nlohmann::json）
- [ ] D2 错误处理中间件：异常 → JSON 错误响应
- [ ] D3 Topic / Card / Chapter 三个核心 GET endpoints
- [ ] D3 后端单测（用 cpp-httplib::Client 自我调用）

**DoD：**
```bash
$ bagu serve &
$ curl http://localhost:8780/api/health
{"ok":true,"version":"0.3.0","schema_version":3}
$ curl http://localhost:8780/api/topics | jq
[{"name":"mysql",...}]
```

#### Sprint 2：前端骨架（Day 4-5）

- [ ] D4 web/ 目录初始化（Vite + React + TS + Tailwind）
- [ ] D4 API client（fetch 封装 + 类型定义）
- [ ] D4 路由 + 4 个页面骨架（Home / Topic / Card / Search）
- [ ] D5 `<MarkdownRenderer>` 组件（marked + highlight.js）
- [ ] D5 列表 / 卡片样式（Tailwind + 响应式）

**DoD：**
- `npm run dev` 跑起来
- 浏览器访问能看到 4 张主题卡片
- 点击进入章节树
- 点击章节看到卡片预览

#### Sprint 3：复习页（Day 6-7）

- [ ] D6 `/api/review/due` + `/api/review/:id/grade` endpoints
- [ ] D6 `<ReviewSession>` 组件 + 键盘事件（空格揭示 / 1-5 评分）
- [ ] D7 移动端触屏：大按钮评分 + 滑动切换
- [ ] D7 进度条 + 答错动画 + 完成总结页

**DoD：**
```bash
$ bagu review                      # CLI TUI 仍能用
$ open http://localhost:8780/review # 浏览器也能用
两个共用同一份 db，进度同步
```

#### Sprint 4：搜索 + 配置（Day 8-9）

- [ ] D8 `/api/search` + `<SearchPage>` 含关键词高亮
- [ ] D8 debounce 输入 / 历史记录（localStorage）
- [ ] D9 `/api/config` GET + PUT
- [ ] D9 `<ConfigPage>` 表单（toml 字段映射）

#### Sprint 5：内嵌 + 发布（Day 10）

- [ ] D10 写 `scripts/gen_embedded_assets.cmake`（前端 dist → C++ 头文件）
- [ ] D10 `bagu serve` 直接 serve 内嵌资源（不依赖外部目录）
- [ ] D10 update README + 截图 + 录 GIF
- [ ] D10 打 tag v0.3.0 + GitHub Release

**v0.3 验收：**
- 单二进制启动后浏览器全部 P0 功能可用
- 手机浏览器同 WiFi 能访问
- 单测覆盖：后端 ≥ 70%、前端关键页面有快照测试

---

### M-Web-2（v0.4.0，5/11 - 5/20）— 完善

#### Sprint 6：AI 面试页（Day 11-13）

- [ ] D11 SSE helper（cpp-httplib chunked content provider）
- [ ] D11-12 `/api/interview/*` 全套 endpoints
- [ ] D12 前端 EventSource 集成 + 流式渲染（typewriter）
- [ ] D13 多轮对话 UI + 评分高亮 + 历史回看

#### Sprint 7：统计页（Day 14-15）

- [ ] D14 `/api/stats/*` endpoints
- [ ] D14 `<Heatmap>` 组件（自实现 SVG，按周排版）
- [ ] D15 主题进度图 / 薄弱列表 / 趋势

#### Sprint 8：体验优化（Day 16-18）

- [ ] D16 暗黑/明亮主题（`prefers-color-scheme`）
- [ ] D16 PWA manifest + service worker（仅缓存静态资源）
- [ ] D17 移动端适配 review / interview
- [ ] D17 收藏夹 / 笔记功能（v0.4 P2）
- [ ] D18 性能优化：bundle 分析 + lazy load + image-loading

#### Sprint 9：发布（Day 19-20）

- [ ] D19 e2e 测试（Playwright，3-5 场景）
- [ ] D19 文档：用户手册 / 部署指南 / Tailscale 配置
- [ ] D20 v0.4.0 tag + Release + 博客文章

**v0.4 验收：**
- AI 面试在浏览器中流式工作正常
- 热力图美观 + 可点击
- PWA 可"加到主屏幕"
- e2e 测试覆盖核心流程

---

## 工作量预估

| 阶段 | 天数 | 主要交付 |
|----|----|----|
| Sprint 1-5 (M-Web-1) | 10 | MVP，4 个页面，能用 |
| Sprint 6-9 (M-Web-2) | 10 | 完善，AI 流式 + 统计 + PWA |
| **合计** | **~20 天**（约 4 周业余） | v0.4.0 完整 Web UI |

---

## 风险

详见 [risks.md](./risks.md) 的 R011-R014（Web UI 相关风险）。

主要：
- 前端学习曲线（缓解：用现成 Tailwind 组件）
- 资源内嵌方案踩坑（缓解：先用文件系统 serve，后期再嵌入）
- 移动端适配工作量超预期（缓解：先 Desktop 再 Mobile）

---

## 不在路线图（明确不做）

- 多用户认证 / 协作
- 推送通知
- 应用商店上架
- 国际化（i18n）
- 中文分词增强（仍走 N-gram）

这些留到 v2.0 或不做。

---

## 相关文档

- [PRD-web-ui.md](../product/PRD-web-ui.md)
- [web-ui-overview.md](../architecture/web-ui-overview.md)
- [http-api-spec.md](../architecture/http-api-spec.md)
- [ADR-0004 / 0005 / 0006](../architecture/adr/README.md)
