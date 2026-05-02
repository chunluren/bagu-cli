/// 注册 service worker（仅在生产构建 + https/localhost 下生效）
///
/// 失败静默：sw 不可用不应阻塞应用本身。
export function registerServiceWorker() {
  if (typeof window === 'undefined') return;
  if (!('serviceWorker' in navigator)) return;
  // dev 模式下的 vite 不要装 sw（HMR 会被 cache 干扰）
  if (import.meta.env.DEV) return;

  window.addEventListener('load', () => {
    navigator.serviceWorker
      .register('/sw.js', { scope: '/' })
      .catch((err) => {
        console.warn('[bagu] service worker register failed:', err);
      });
  });
}
