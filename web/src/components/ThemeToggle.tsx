import { Sun, Moon, Monitor } from 'lucide-react';

import { useTheme, type Theme } from '../hooks/useTheme';

const OPTIONS: { value: Theme; icon: React.ComponentType<{ size?: number }>; label: string }[] = [
  { value: 'light', icon: Sun, label: '亮色' },
  { value: 'dark', icon: Moon, label: '暗色' },
  { value: 'system', icon: Monitor, label: '跟随系统' },
];

/// 顶部主题切换：3 段开关
export function ThemeToggle() {
  const [theme, setTheme] = useTheme();
  return (
    <div
      role="radiogroup"
      aria-label="主题"
      className="flex items-center border border-slate-200 dark:border-slate-700 rounded-md p-0.5"
    >
      {OPTIONS.map(({ value, icon: Icon, label }) => (
        <button
          key={value}
          role="radio"
          aria-checked={theme === value}
          onClick={() => setTheme(value)}
          title={label}
          className={`p-1.5 rounded-sm transition ${
            theme === value
              ? 'bg-bagu-accent text-white'
              : 'text-slate-500 hover:text-bagu-accent'
          }`}
        >
          <Icon size={14} />
        </button>
      ))}
    </div>
  );
}
