import { Link } from 'react-router-dom';
import { Layers, FileText, BookOpen, Brain, Sparkles } from 'lucide-react';

import { api } from '../api/client';
import type { Topic, DueSummary } from '../api/types';
import { useApi } from '../hooks/useApi';

export function HomePage() {
  const topics = useApi<Topic[]>(() => api.get('/api/topics'));
  const due    = useApi<DueSummary>(() => api.get('/api/review/due-summary'));

  if (topics.loading) return <p className="text-slate-500">加载中...</p>;
  if (topics.error) {
    return (
      <div className="text-rose-600">
        <p>加载失败：{topics.error.message}</p>
        <p className="text-sm text-slate-500 mt-2">
          请确认 <code className="bg-slate-100 dark:bg-slate-800 px-1.5 py-0.5 rounded">bagu serve</code>{' '}
          在运行。
        </p>
      </div>
    );
  }
  if (!topics.data || topics.data.length === 0) {
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
      {/* 今日到期 banner */}
      {due.data && (due.data.total_due > 0 || due.data.total_new > 0) ? (
        <Link
          to="/review"
          className="block mb-5 border border-bagu-accent/40 bg-bagu-accent/5 hover:bg-bagu-accent/10 rounded-lg p-4 transition"
        >
          <div className="flex items-center justify-between gap-4">
            <div className="flex items-center gap-3">
              <Brain size={22} className="text-bagu-accent flex-shrink-0" />
              <div>
                <div className="font-semibold">
                  今日到期{' '}
                  <span className="text-bagu-accent font-mono">
                    {due.data.total_due}
                  </span>{' '}
                  张
                  {due.data.total_new > 0 && (
                    <>
                      {' '}+{' '}
                      <span className="text-amber-600 font-mono">
                        {due.data.total_new}
                      </span>{' '}
                      张新卡
                    </>
                  )}
                </div>
                <div className="text-xs text-slate-500 mt-0.5">
                  {due.data.items
                    .slice(0, 3)
                    .map((t) => `${t.topic_name} ${t.due + t.new_cards}`)
                    .join('  ·  ')}
                  {due.data.items.length > 3 && '  ·  ...'}
                </div>
              </div>
            </div>
            <span className="text-sm text-bagu-accent font-medium">
              开始复习 →
            </span>
          </div>
        </Link>
      ) : due.data ? (
        <div className="mb-5 border border-emerald-300 dark:border-emerald-800 bg-emerald-50 dark:bg-emerald-900/20 rounded-lg p-4 flex items-center gap-3">
          <Sparkles size={20} className="text-emerald-600 flex-shrink-0" />
          <div className="text-sm">
            🎉 今日没有到期卡片。
            <Link
              to="/interview"
              className="text-emerald-700 dark:text-emerald-400 hover:underline ml-2"
            >
              试试 AI 面试 →
            </Link>
          </div>
        </div>
      ) : null}

      <h1 className="text-2xl font-semibold mb-4">主题</h1>
      <div className="grid grid-cols-1 sm:grid-cols-2 lg:grid-cols-3 gap-3">
        {topics.data.map((t) => (
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
        共 {topics.data.length} 个主题，
        {topics.data.reduce((s, t) => s + (t.cards ?? 0), 0)} 张卡片
      </div>
    </div>
  );
}
