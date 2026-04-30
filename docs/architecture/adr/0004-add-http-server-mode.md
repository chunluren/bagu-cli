# ADR-0004: 增加 HTTP server 模式（`bagu serve`）

- **状态**: Proposed
- **日期**: 2026-04-30
- **作者**: Li Yan

## 背景

bagu-cli 当前是纯 CLI/TUI 工具。用户提出在以下场景希望有 Web 入口：
- 手机浏览器（电脑 + 手机同 WiFi）
- 桌面浏览器（与 IDE / ChatGPT 并排）
- 截图分享更友好（无 ANSI）

约束：
- 不引入云端依赖
- 不破坏 CLI 现有用法
- 不引入大型框架

## 决策

**给 bagu-cli 新增 `bagu serve` 命令，启动嵌入式 HTTP server，单进程提供：**
- REST API（基于现有 service 层）
- 内嵌的 React SPA 静态资源
- SSE 流式接口（用于 LLM 流）

## 备选方案

| 方案 | 优点 | 缺点 | 不选理由 |
|----|----|----|----|
| **嵌入式 HTTP server**（当前选） | 单二进制、零部署 | 增加 ~50KB 二进制 | — |
| 独立后端进程（Node/Go）| 前后端解耦 | 引入新语言 | 偏离 C++ 项目定位 |
| Tauri 桌面应用 | 真桌面 App | Rust 学习成本，包体积大 | 工作量大，先做 web |
| Electron | 生态成熟 | 50-100MB 体积，性能差 | 反 bagu-cli 定位 |
| 纯 CLI（不做 Web） | 维持现状 | 多端体验差 | 错过手机使用场景 |

## 后果

### 正面

- 单进程：用户只需要一个二进制（含前端资源）
- 跨设备：浏览器即可访问
- 复用现有 service 层 — 0 业务逻辑重写
- 数据仍本地（隐私零泄漏）

### 负面

- 二进制变大（前端 bundle ~200KB + cpp-httplib ~30KB）
- 引入网络层考量（端口 / 鉴权 / CORS）
- 维护多了前端项目（web/ 子目录）

### 中性

- 默认绑定 127.0.0.1，需要时 `--bind 0.0.0.0` + `--token` 才公开
- Vite 前端 dev server 不打包到 bagu，仅开发时用

## 实施

详见 [planning/web-ui-roadmap.md](../../planning/web-ui-roadmap.md)：
- v0.3.0 实现 P0（启动 + 主题/章节/卡片/搜索/复习）
- v0.4.0 实现 P1（统计/AI 面试/配置）

## 相关链接

- [PRD-web-ui.md](../../product/PRD-web-ui.md)
- [web-ui-overview.md](../web-ui-overview.md)
- [http-api-spec.md](../http-api-spec.md)
- [ADR-0005 cpp-httplib](./0005-choose-cpp-httplib.md)
- [ADR-0006 frontend stack](./0006-frontend-stack-vite-react.md)
