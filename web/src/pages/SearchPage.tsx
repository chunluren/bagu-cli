import { useEffect, useState, type ReactElement } from 'react';
import { Link } from 'react-router-dom';
import { Search, Lightbulb } from 'lucide-react';

import { api } from '../api/client';
import type { SearchResult, Topic } from '../api/types';
import { useApi } from '../hooks/useApi';

function highlight(text: string, kw: string): ReactElement {
  if (!kw) return <>{text}</>;
  const lower = text.toLowerCase();
  const k = kw.toLowerCase();
  const parts: ReactElement[] = [];
  let i = 0;
  while (true) {
    const idx = lower.indexOf(k, i);
    if (idx < 0) {
      parts.push(<span key={i}>{text.slice(i)}</span>);
      break;
    }
    if (idx > i) parts.push(<span key={i}>{text.slice(i, idx)}</span>);
    parts.push(<mark key={idx} className="hl">{text.slice(idx, idx + kw.length)}</mark>);
    i = idx + kw.length;
  }
  return <>{parts}</>;
}

function snippet(text: string, kw: string, max = 120): string {
  if (!text) return '';
  const cleaned = text.replace(/\n+/g, ' ').replace(/\s+/g, ' ');
  if (!kw) return cleaned.slice(0, max);
  const lower = cleaned.toLowerCase();
  const idx = lower.indexOf(kw.toLowerCase());
  if (idx < 0) return cleaned.slice(0, max);
  const start = Math.max(0, idx - 30);
  const end = Math.min(cleaned.length, idx + kw.length + max - 30);
  return (start > 0 ? '...' : '') + cleaned.slice(start, end) + (end < cleaned.length ? '...' : '');
}

// 一些常见搜索示例，初次访问时引导
const EXAMPLE_QUERIES = [
  'MVCC', 'epoll', 'B+ 树', '隔离级别', 'Redis 持久化',
  '哈希表', '虚函数', '锁', '事务', '索引下推',
];

export function SearchPage() {
  const [q, setQ] = useState('');
  const [debounced, setDebounced] = useState('');
  const [result, setResult] = useState<SearchResult | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);
  const topics = useApi<Topic[]>(() => api.get('/api/topics'));

  useEffect(() => {
    const id = setTimeout(() => setDebounced(q.trim()), 250);
    return () => clearTimeout(id);
  }, [q]);

  useEffect(() => {
    if (!debounced) {
      setResult(null);
      setError(null);
      return;
    }
    setLoading(true);
    setError(null);
    api
      .get<SearchResult>(`/api/search?q=${encodeURIComponent(debounced)}&limit=20`)
      .then((data) => {
        setResult(data);
        setLoading(false);
      })
      .catch((e: Error) => {
        setError(e.message);
        setLoading(false);
      });
  }, [debounced]);

  return (
    <div>
      <h1 className="text-2xl font-semibold mb-4">搜索</h1>

      <div className="relative mb-6">
        <Search
          size={16}
          className="absolute left-3 top-1/2 -translate-y-1/2 text-slate-400"
        />
        <input
          type="text"
          autoFocus
          value={q}
          onChange={(e) => setQ(e.target.value)}
          placeholder='输入关键词，如 "MVCC" / "epoll" / "B+ 树"'
          className="w-full pl-9 pr-3 py-2 border border-slate-300 dark:border-slate-600 dark:bg-slate-800 rounded-md focus:outline-none focus:border-bagu-accent"
        />
      </div>

      {loading && <p className="text-slate-500">搜索中...</p>}
      {error && <p className="text-rose-600">{error}</p>}

      {result && (
        <>
          <div className="text-xs text-slate-500 mb-3">
            找到 {result.items.length} 条 · 用时 {result.took_ms}ms
          </div>
          <div className="space-y-3">
            {result.items.map((c) => (
              <Link
                key={c.id}
                to={`/cards/${c.id}`}
                className="block border border-slate-200 dark:border-slate-700 rounded-md p-3 hover:border-bagu-accent transition"
              >
                <div className="text-xs text-slate-500 mb-1">
                  card#{c.id} · {c.card_type}
                </div>
                <div className="font-medium">
                  {highlight(c.question, debounced)}
                </div>
                <div className="text-sm text-slate-500 mt-1 line-clamp-2">
                  {highlight(snippet(c.answer, debounced), debounced)}
                </div>
              </Link>
            ))}
          </div>
        </>
      )}

      {!loading && !error && !result && q && (
        <p className="text-slate-500 text-sm">输入关键词后开始搜索...</p>
      )}

      {/* 初次访问 — 没输入任何内容时显示引导 */}
      {!loading && !error && !result && !q && (
        <div className="text-sm space-y-4">
          {topics.data && topics.data.length > 0 && (
            <div className="flex items-start gap-2 text-slate-500">
              <Lightbulb size={14} className="mt-0.5 flex-shrink-0 text-amber-500" />
              <div>
                共 {topics.data.length} 个主题，
                {topics.data.reduce((s, t) => s + (t.cards ?? 0), 0)} 张卡片可搜。
                FTS5 全文索引，毫秒级响应。
              </div>
            </div>
          )}
          <div>
            <div className="text-xs text-slate-400 mb-2">试试这些：</div>
            <div className="flex flex-wrap gap-2">
              {EXAMPLE_QUERIES.map((ex) => (
                <button
                  key={ex}
                  onClick={() => setQ(ex)}
                  className="px-2.5 py-1 text-xs border border-slate-200 dark:border-slate-700 rounded-full hover:border-bagu-accent hover:text-bagu-accent transition"
                >
                  {ex}
                </button>
              ))}
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
