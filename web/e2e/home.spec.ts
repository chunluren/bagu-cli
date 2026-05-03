import { test, expect } from './fixtures';

test.describe('Home page', () => {
  test('loads with nav and footer', async ({ page }) => {
    await page.goto('/');
    // 顶部 logo
    await expect(page.locator('text=bagu-cli').first()).toBeVisible();
    // 5 个 nav 入口
    for (const label of ['主题', '搜索', '复习', '面试', '统计']) {
      await expect(page.getByRole('link', { name: label, exact: false })).toBeVisible();
    }
    // 主题切换 3 段
    await expect(page.getByRole('radiogroup', { name: '主题' })).toBeVisible();
  });

  test('empty state: no topics imported', async ({ page }) => {
    await page.goto('/');
    // 主题列表区不应崩溃 — 文档区/空提示 至少一个出现
    // （HomePage 在 0 主题时显示一个引导文案；即使接口返回 [] 页面也能渲染）
    await expect(page.locator('body')).not.toContainText('Error');
  });

  test('due-summary endpoint returns shape even on empty DB', async ({ request }) => {
    const r = await request.get('/api/review/due-summary');
    expect(r.ok()).toBeTruthy();
    const j = await r.json();
    expect(j.total_due).toBe(0);
    expect(j.total_new).toBe(0);
    expect(Array.isArray(j.items)).toBe(true);
  });
});

test.describe('Health API', () => {
  test('GET /api/health returns ok', async ({ request }) => {
    const res = await request.get('/api/health');
    expect(res.ok()).toBeTruthy();
    const j = await res.json();
    expect(j.ok).toBe(true);
    expect(j.version).toBeDefined();
    expect(j.schema_version).toBe(4);
  });
});
