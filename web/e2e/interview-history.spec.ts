import { test, expect } from './fixtures';

test.describe('Interview history', () => {
  test('empty state shows CTA', async ({ page }) => {
    await page.goto('/interview/history');
    await expect(page.getByText('面试历史').first()).toBeVisible();
    // 空 DB 应该看到引导文案
    await expect(page.getByText('还没有面试会话')).toBeVisible();
    await expect(page.getByRole('link', { name: /开始第一次模拟面试/ })).toBeVisible();
  });

  test('history link visible from /interview setup', async ({ page }) => {
    await page.goto('/interview');
    await expect(page.getByRole('link', { name: /查看历史会话/ })).toBeVisible();
  });

  test('detail page handles unknown session id gracefully', async ({ page }) => {
    await page.goto('/interview/sessions/99999');
    // 后端 404，前端 useApi 把 error 显示出来 — 不要白屏
    await expect(page.locator('body')).toContainText(/session|not found|未找到|加载/i);
    // 顶部 nav 仍渲染
    await expect(page.locator('text=bagu-cli').first()).toBeVisible();
  });
});
