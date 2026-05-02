import { useEffect, useState } from 'react';

export type Theme = 'light' | 'dark' | 'system';

const STORAGE_KEY = 'bagu-theme';

function getInitial(): Theme {
  if (typeof localStorage === 'undefined') return 'system';
  const saved = localStorage.getItem(STORAGE_KEY) as Theme | null;
  return saved === 'light' || saved === 'dark' || saved === 'system'
    ? saved
    : 'system';
}

function applyTheme(t: Theme) {
  if (typeof document === 'undefined') return;
  const root = document.documentElement;
  const dark =
    t === 'dark' ||
    (t === 'system' &&
      window.matchMedia('(prefers-color-scheme: dark)').matches);
  root.classList.toggle('dark', dark);
}

/// 主题三态：light / dark / system
///
/// 持久化到 localStorage；system 模式跟随 prefers-color-scheme 实时变化。
export function useTheme(): [Theme, (t: Theme) => void] {
  const [theme, setTheme] = useState<Theme>(getInitial);

  useEffect(() => {
    applyTheme(theme);
    if (theme === 'system') {
      const mq = window.matchMedia('(prefers-color-scheme: dark)');
      const h = () => applyTheme('system');
      mq.addEventListener('change', h);
      return () => mq.removeEventListener('change', h);
    }
  }, [theme]);

  const update = (t: Theme) => {
    localStorage.setItem(STORAGE_KEY, t);
    setTheme(t);
  };

  return [theme, update];
}

/// 在 React 挂载前同步 dark class，避免首屏闪白
export function bootstrapTheme() {
  applyTheme(getInitial());
}
