import type { ReactNode } from 'react';
import { Link, NavLink } from 'react-router-dom';
import { Home, Search, Brain, BarChart3, Bot } from 'lucide-react';

interface Props {
  children: ReactNode;
}

const navItems = [
  { to: '/', icon: Home, label: '主题' },
  { to: '/search', icon: Search, label: '搜索' },
  { to: '/review', icon: Brain, label: '复习' },
  { to: '/interview', icon: Bot, label: '面试' },
  { to: '/stats', icon: BarChart3, label: '统计' },
];

export function AppLayout({ children }: Props) {
  return (
    <div className="flex flex-col min-h-screen">
      <header className="border-b border-slate-200 dark:border-slate-700 sticky top-0 bg-white/90 dark:bg-slate-900/90 backdrop-blur z-10">
        <div className="max-w-6xl mx-auto px-4 py-3 flex items-center gap-6">
          <Link to="/" className="font-mono text-lg font-semibold">
            <span className="text-bagu-accent">bagu</span>-cli
          </Link>
          <nav className="flex items-center gap-1">
            {navItems.map(({ to, icon: Icon, label }) => (
              <NavLink
                key={to}
                to={to}
                end={to === '/'}
                className={({ isActive }) =>
                  `flex items-center gap-1.5 px-3 py-1.5 rounded text-sm transition ${
                    isActive
                      ? 'bg-bagu-accent/15 text-bagu-accent'
                      : 'text-slate-600 dark:text-slate-300 hover:bg-slate-100 dark:hover:bg-slate-800'
                  }`
                }
              >
                <Icon size={16} />
                {label}
              </NavLink>
            ))}
          </nav>
        </div>
      </header>
      <main className="flex-1 max-w-6xl w-full mx-auto px-4 py-6">{children}</main>
      <footer className="border-t border-slate-200 dark:border-slate-700 text-center text-xs text-slate-500 py-3">
        bagu-cli · 数据本地存储 · <a className="hover:underline" href="https://github.com/chunluren/bagu-cli">GitHub</a>
      </footer>
    </div>
  );
}
