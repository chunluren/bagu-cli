# Playwright e2e

> 跨前后端的端到端测试。配置 `web/playwright.config.ts`，用例 `web/e2e/*.spec.ts`。

## 跑测前置条件

```bash
# 1. 必须先构建 C++ binary（自动嵌入 web/dist）
cd ../web && npm run build && cd ..
cmake --build build -j

# 2. 安装 chromium（首次跑，~150MB）
cd web
npx playwright install chromium
```

## 跑

```bash
cd web

# headless（默认）
npm run e2e

# UI 模式（交互式调试）
npm run e2e:ui

# 单个 spec
npx playwright test e2e/theme.spec.ts

# 失败时自动截图、保留 trace
# 报告：playwright-report/index.html
```

## 用例覆盖（v0.4 基线）

| 文件 | 覆盖 |
|---|---|
| `home.spec.ts` | 首页 nav / 主题切换可见 / `/api/health` |
| `theme.spec.ts` | 三态主题切换 / localStorage 持久化 / 刷新后保留 |
| `navigation.spec.ts` | 5 条路由渲染 / SPA 404 fallback |
| `stats.spec.ts` | 4 张统计卡 / 热力图 days 切换 / 薄弱卡片空态 |
| `api-smoke.spec.ts` | stats / interview / PWA assets |

## 数据隔离

`playwright.config.ts` 启动 `bagu serve` 子进程，把 `BAGU_HOME` 指向 `/tmp/bagu-e2e`。
不会动到本地 `~/.bagu/`。

## CI

`.github/workflows/ci.yml` 的 `e2e` job 在 `web-build` + `build-test` 之后跑。
不阻塞主流水（`continue-on-error` 起步阶段；稳定后改为强制）。

## 故障

| 症状 | 排查 |
|---|---|
| `bagu serve` 起不来 | 检查 `../build/src/bagu` 存在 + `web/dist` 已嵌入 |
| port 18910 被占 | `E2E_PORT=29000 npm run e2e` |
| 首页空白 | binary 没嵌 web；rebuild C++ |
| 测试 flaky | `playwright.config.ts` `fullyParallel: false` 已串行；如再 flaky 看 trace |
