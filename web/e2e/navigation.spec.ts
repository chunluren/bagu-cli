import { test, expect } from './fixtures';

test.describe('Navigation between pages', () => {
  const routes: { path: string; mustContain: RegExp | string }[] = [
    { path: '/',          mustContain: 'bagu-cli' },
    { path: '/search',    mustContain: /搜索|search/i },
    { path: '/review',    mustContain: /复习|review/i },
    { path: '/interview', mustContain: /面试|interview/i },
    { path: '/stats',     mustContain: /统计|stats/i },
  ];

  for (const r of routes) {
    test(`renders ${r.path}`, async ({ page }) => {
      await page.goto(r.path);
      // SPA 应正常路由 + 渲染（不应是 raw 404 / 白屏）
      await expect(page.locator('body')).toContainText(r.mustContain);
      // 顶部 nav 始终存在
      await expect(page.locator('text=bagu-cli').first()).toBeVisible();
    });
  }

  test('unknown route falls back to SPA shell', async ({ page }) => {
    const res = await page.goto('/this-does-not-exist');
    expect(res?.status()).toBe(200);   // 后端 SPA fallback
    await expect(page.locator('text=bagu-cli').first()).toBeVisible();
  });
});
