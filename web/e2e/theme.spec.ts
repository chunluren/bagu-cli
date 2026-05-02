import { test, expect } from './fixtures';

test.describe('Theme toggle', () => {
  test('toggles light/dark/system; persists to localStorage', async ({ page }) => {
    await page.goto('/');
    const html = page.locator('html');

    // 找到 3 段主题按钮（按 title 区分）
    const lightBtn = page.getByRole('radio', { name: /亮色/ });
    const darkBtn  = page.getByRole('radio', { name: /暗色/ });
    const sysBtn   = page.getByRole('radio', { name: /跟随系统/ });

    // 点 dark
    await darkBtn.click();
    await expect(html).toHaveClass(/dark/);

    // localStorage 持久化
    const stored = await page.evaluate(() => localStorage.getItem('bagu-theme'));
    expect(stored).toBe('dark');

    // 切 light
    await lightBtn.click();
    await expect(html).not.toHaveClass(/dark/);
    expect(await page.evaluate(() => localStorage.getItem('bagu-theme'))).toBe('light');

    // 切 system（在 chromium 默认非 dark 偏好下，html 不应含 dark）
    await sysBtn.click();
    expect(await page.evaluate(() => localStorage.getItem('bagu-theme'))).toBe('system');
  });

  test('survives page reload', async ({ page }) => {
    await page.goto('/');
    await page.getByRole('radio', { name: /暗色/ }).click();
    await page.reload();
    await expect(page.locator('html')).toHaveClass(/dark/);
  });
});
