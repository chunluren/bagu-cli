import { test, expect } from './fixtures';

test.describe('Stats page', () => {
  test('renders all 4 stat cards even on empty DB', async ({ page }) => {
    await page.goto('/stats');
    await expect(page.getByText('连续打卡')).toBeVisible();
    await expect(page.getByText('累计复习')).toBeVisible();
    await expect(page.getByText('正确率')).toBeVisible();
    await expect(page.getByText('近 30 天活跃')).toBeVisible();
  });

  test('heatmap range buttons switch days param', async ({ page }) => {
    await page.goto('/stats');

    const btn30 = page.getByRole('button', { name: '30 天' });
    const btn180 = page.getByRole('button', { name: '180 天' });

    // waitForResponse 必须在 click 前注册（监听并发请求）
    const r30 = page.waitForResponse(
      (resp) => /\/api\/stats\/heatmap\?days=30/.test(resp.url())
    );
    await btn30.click();
    await r30;

    const r180 = page.waitForResponse(
      (resp) => /\/api\/stats\/heatmap\?days=180/.test(resp.url())
    );
    await btn180.click();
    await r180;
  });

  test('weak cards section renders empty-state', async ({ page }) => {
    await page.goto('/stats');
    // 0 复习历史 → "没有薄弱卡片" 文案存在（heading 标题也叫薄弱卡片，所以用 .first()）
    await expect(
      page.getByText(/没有薄弱卡片，太棒了/)
    ).toBeVisible();
  });
});
