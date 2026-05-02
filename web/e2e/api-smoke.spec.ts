import { test, expect } from './fixtures';

/// 后端接口烟测：覆盖 v0.4 新加的 stats / interview 端点入参校验
test.describe('API smoke (v0.4)', () => {
  test('GET /api/stats/overall returns counts', async ({ request }) => {
    const r = await request.get('/api/stats/overall');
    expect(r.ok()).toBeTruthy();
    const j = await r.json();
    expect(j.total_topics).toBeDefined();
    expect(j.total_cards).toBeDefined();
    expect(j.streak_days).toBeDefined();
  });

  test('GET /api/stats/heatmap?days=14 returns 14 items', async ({ request }) => {
    const r = await request.get('/api/stats/heatmap?days=14');
    expect(r.ok()).toBeTruthy();
    const j = await r.json();
    expect(j.days).toBe(14);
    expect(j.items.length).toBe(14);
  });

  test('POST /api/interview/sessions rejects missing topic', async ({ request }) => {
    const r = await request.post('/api/interview/sessions', {
      data: { question_count: 3 },
    });
    expect(r.status()).toBe(400);
    const j = await r.json();
    expect(j.error.message).toMatch(/topic/);
  });

  test('GET /api/interview/sessions empty list', async ({ request }) => {
    const r = await request.get('/api/interview/sessions');
    expect(r.ok()).toBeTruthy();
    const j = await r.json();
    expect(Array.isArray(j.items)).toBe(true);
  });
});

/// PWA 资源
test.describe('PWA assets', () => {
  test('manifest.webmanifest with correct mime', async ({ request }) => {
    const r = await request.get('/manifest.webmanifest');
    expect(r.ok()).toBeTruthy();
    expect(r.headers()['content-type']).toMatch(/manifest\+json/);
    const j = await r.json();
    expect(j.name).toMatch(/bagu/);
  });

  test('sw.js served as javascript', async ({ request }) => {
    const r = await request.get('/sw.js');
    expect(r.ok()).toBeTruthy();
    expect(r.headers()['content-type']).toMatch(/javascript/);
    const body = await r.text();
    expect(body).toMatch(/service worker/i);
  });

  test('icons served', async ({ request }) => {
    for (const path of ['/favicon.svg', '/icon-192.png', '/icon-512.png', '/apple-touch-icon.png']) {
      const r = await request.get(path);
      expect(r.ok(), `${path} not found`).toBeTruthy();
    }
  });
});
