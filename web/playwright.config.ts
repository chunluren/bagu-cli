import { defineConfig, devices } from '@playwright/test';

// 让 node 的 fetch / undici 不要把 localhost 走系统代理（Clash / shadowsocks 等）
// 必须在 import 前生效，但 process.env 改动 import 时已迟。这里用静态赋值兜底。
process.env.NO_PROXY = process.env.NO_PROXY
  ? `${process.env.NO_PROXY},localhost,127.0.0.1`
  : 'localhost,127.0.0.1';

const PORT = parseInt(process.env.E2E_PORT || '18910', 10);
const HOST = process.env.E2E_HOST || '127.0.0.1';
const BASE_URL = `http://${HOST}:${PORT}`;

/// e2e 配置
///
/// 启动模式：
///   - webServer 段会启动 `bagu serve` 子进程，跑完测试后自动关闭
///   - BAGU_HOME 指向临时目录，避免污染本地 ~/.bagu/
///
/// 期望预先：
///   - 当前目录的父级（仓库根）已构建过：build/src/bagu 存在并嵌入 web 前端
///   - 见 e2e/README.md
export default defineConfig({
  testDir: './e2e',
  timeout: 30_000,
  expect: { timeout: 5_000 },
  fullyParallel: false,    // serve 是单进程共享 db，串行更稳
  retries: process.env.CI ? 1 : 0,
  workers: 1,
  reporter: process.env.CI ? [['list'], ['html', { open: 'never' }]] : 'list',

  use: {
    baseURL: BASE_URL,
    trace: 'retain-on-failure',
    screenshot: 'only-on-failure',
    video: 'off',
    locale: 'zh-CN',
    ignoreHTTPSErrors: true,
    // 浏览器侧禁用系统代理 — 否则 Clash 之类会拦 localhost
    launchOptions: {
      args: ['--no-proxy-server', '--proxy-bypass-list=<-loopback>'],
    },
  },

  projects: [
    { name: 'chromium', use: { ...devices['Desktop Chrome'] } },
  ],

  webServer: {
    command: `${process.env.BAGU_BIN || '../build/src/bagu'} serve --port ${PORT} --bind ${HOST}`,
    env: {
      // 用临时目录，不碰真实数据
      BAGU_HOME: process.env.E2E_BAGU_HOME || '/tmp/bagu-e2e',
    },
    url: `${BASE_URL}/api/health`,
    reuseExistingServer: !process.env.CI,
    timeout: 30_000,
    stdout: 'pipe',
    stderr: 'pipe',
  },
});
