// bagu-cli minimal service worker
//
// Strategy:
//   - app shell (HTML / JS / CSS / 字体): cache-first, fallback network
//   - /api/*: network-only (永远拿最新数据，离线时直接 fail)
//   - 导航请求 (mode=navigate): 网络失败时回退缓存的 / (SPA shell)
//
// 升级：发新版时改 CACHE_VERSION，旧 cache 在 activate 时清掉

const CACHE_VERSION = 'v1';
const SHELL_CACHE = `bagu-shell-${CACHE_VERSION}`;
const ASSETS_CACHE = `bagu-assets-${CACHE_VERSION}`;

const SHELL_URLS = ['/', '/index.html', '/manifest.webmanifest', '/favicon.svg'];

self.addEventListener('install', (event) => {
  event.waitUntil(
    caches.open(SHELL_CACHE).then((c) => c.addAll(SHELL_URLS)).catch(() => {})
  );
  self.skipWaiting();
});

self.addEventListener('activate', (event) => {
  event.waitUntil(
    caches.keys().then((keys) =>
      Promise.all(
        keys
          .filter((k) => k !== SHELL_CACHE && k !== ASSETS_CACHE && k.startsWith('bagu-'))
          .map((k) => caches.delete(k))
      )
    )
  );
  self.clients.claim();
});

self.addEventListener('fetch', (event) => {
  const req = event.request;
  if (req.method !== 'GET') return;

  const url = new URL(req.url);

  // /api/* → 网络专用
  if (url.pathname.startsWith('/api/')) return;

  // 跨源 → 不接管
  if (url.origin !== self.location.origin) return;

  // 导航请求：network-first，离线 fallback /
  if (req.mode === 'navigate') {
    event.respondWith(
      fetch(req).catch(() =>
        caches.match('/').then((r) => r || caches.match('/index.html'))
      )
    );
    return;
  }

  // 静态资源：cache-first，命中即返；否则网络 + 写缓存
  event.respondWith(
    caches.match(req).then((cached) => {
      if (cached) return cached;
      return fetch(req).then((res) => {
        if (res.ok && (res.type === 'basic' || res.type === 'opaque')) {
          const copy = res.clone();
          caches.open(ASSETS_CACHE).then((c) => c.put(req, copy)).catch(() => {});
        }
        return res;
      });
    })
  );
});

self.addEventListener('message', (e) => {
  if (e.data === 'SKIP_WAITING') self.skipWaiting();
});
