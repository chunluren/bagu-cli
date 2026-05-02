import { Link } from 'react-router-dom';
import { Layers, FileText, BookOpen } from 'lucide-react';

import { api } from '../api/client';
import type { Topic } from '../api/types';
import { useApi } from '../hooks/useApi';

export function HomePage() {
  const { data, error, loading } = useApi<Topic[]>(() => api.get('/api/topics'));

  if (loading) return <p className="text-slate-500">加载中...</p>;
  if (error) {
    return (
      <div className="text-rose-600">
        <p>加载失败：{error.message}</p>
        <p className="text-sm text-slate-500 mt-2">
          请确认 <code className="bg-slate-100 dark:bg-slate-800 px-1.5 py-0.5 rounded">bagu serve</code>{' '}
          在运行。
        </p>
      </div>
    );
  }
  if (!data || data.length === 0) {
    return (
      <div className="text-slate-500">
        <p>还没有导入任何主题。</p>
        <p className="text-sm mt-2">
          请在终端运行：
          <code className="bg-slate-100 dark:bg-slate-800 px-1.5 py-0.5 rounded ml-1">
            bagu import &lt;path&gt;
          </code>
        </p>
      </div>
    );
  }

  return (
    <div>
      <h1 className="text-2xl font-semibold mb-4">主题</h1>
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-3">
        {data.map((t) => (
          <Link
            key={t.id}
            to={`/topics/${t.name}`}
            className="block border border-slate-200 dark:border-slate-700 rounded-lg p-4 hover:border-bagu-accent hover:shadow-sm transition"
          >
            <div className="font-mono text-bagu-accent text-sm">{t.name}</div>
            <div className="font-medium mt-1 truncate">{t.title}</div>
            <div className="flex items-center gap-4 mt-3 text-xs text-slate-500">
              <span className="flex items-center gap-1">
                <FileText size={12} /> {t.cards ?? 0} cards
              </span>
              <span className="flex items-center gap-1">
                <Layers size={12} /> {t.chapters ?? 0} chapters
              </span>
            </div>
          </Link>
        ))}
      </div>
      <div className="mt-6 text-xs text-slate-500 flex items-center gap-1.5">
        <BookOpen size={12} />
        共 {data.length} 个主题，
        {data.reduce((s, t) => s + (t.cards ?? 0), 0)} 张卡片
      </div>
    </div>
  );
}
