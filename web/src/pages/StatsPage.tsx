import { useState } from 'react';
import { Link } from 'react-router-dom';
import { Flame, BookOpen, Target, AlertTriangle, BarChart3 } from 'lucide-react';

import { api } from '../api/client';
import type {
  HeatmapResponse,
  OverallStats,
  TopicProgress,
  WeakCard,
} from '../api/types';
import { useApi } from '../hooks/useApi';
import { Heatmap } from '../components/Heatmap';

export function StatsPage() {
  const [days, setDays] = useState(90);

  const overall = useApi<OverallStats>(() => api.get('/api/stats/overall'), []);
  const topics = useApi<{ items: TopicProgress[] }>(
    () => api.get('/api/stats/topics'),
    []
  );
  const heatmap = useApi<HeatmapResponse>(
    () => api.get(`/api/stats/heatmap?days=${days}`),
    [days]
  );
  const weak = useApi<{ items: WeakCard[] }>(
    () => api.get('/api/stats/weak?recent=200&top=10'),
    []
  );

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-2xl font-semibold flex items-center gap-2">
          <BarChart3 size={24} className="text-bagu-accent" />
          学习统计
        </h1>
      </div>

      {/* 总览 */}
      <section>
        {overall.loading && <p className="text-sm text-slate-500">加载中...</p>}
        {overall.error && (
          <p className="text-sm text-rose-600">{overall.error.message}</p>
        )}
        {overall.data && (
          <div className="grid grid-cols-2 sm:grid-cols-4 gap-3">
            <StatCard
              icon={<Flame size={16} className="text-orange-500" />}
              label="连续打卡"
              value={`${overall.data.streak_days} 天`}
            />
            <StatCard
              icon={<BookOpen size={16} className="text-bagu-accent" />}
              label="累计复习"
              value={overall.data.total_reviews.toLocaleString()}
              hint={`${overall.data.learned_unique_cards} 张卡`}
            />
            <StatCard
              icon={<Target size={16} className="text-emerald-600" />}
              label="正确率"
              value={`${(overall.data.overall_accuracy * 100).toFixed(0)}%`}
              hint={`${overall.data.total_correct}/${overall.data.total_reviews}`}
            />
            <StatCard
              icon={<BarChart3 size={16} className="text-violet-600" />}
              label="近 30 天活跃"
              value={`${overall.data.active_days_30} 天`}
            />
          </div>
        )}
      </section>

      {/* 热力图 */}
      <section className="border border-slate-200 dark:border-slate-700 rounded-lg p-4">
        <div className="flex items-center justify-between mb-3">
          <h2 className="text-base font-semibold">复习热力图</h2>
          <div className="flex gap-1 text-xs">
            {[30, 90, 180, 365].map((d) => (
              <button
                key={d}
                onClick={() => setDays(d)}
                className={`px-2 py-1 rounded ${
                  days === d
                    ? 'bg-bagu-accent text-white'
                    : 'text-slate-500 hover:bg-slate-100 dark:hover:bg-slate-800'
                }`}
              >
                {d} 天
              </button>
            ))}
          </div>
        </div>
        {heatmap.loading && <p className="text-sm text-slate-500">加载中...</p>}
        {heatmap.error && (
          <p className="text-sm text-rose-600">{heatmap.error.message}</p>
        )}
        {heatmap.data && (
          <Heatmap
            items={heatmap.data.items}
            hrefForDate={() => '/review?max_new=20'}
          />
        )}
      </section>

      {/* 各主题进度 */}
      <section className="border border-slate-200 dark:border-slate-700 rounded-lg p-4">
        <h2 className="text-base font-semibold mb-3">各主题进度</h2>
        {topics.loading && <p className="text-sm text-slate-500">加载中...</p>}
        {topics.error && (
          <p className="text-sm text-rose-600">{topics.error.message}</p>
        )}
        {topics.data && topics.data.items.length === 0 && (
          <p className="text-sm text-slate-500">还没导入主题。</p>
        )}
        {topics.data && topics.data.items.length > 0 && (
          <ul className="space-y-2">
            {topics.data.items.map((t) => {
              const pct = t.total_cards > 0
                ? Math.round((t.learned_cards / t.total_cards) * 100)
                : 0;
              return (
                <li
                  key={t.topic_id}
                  className="text-sm border-b border-slate-100 dark:border-slate-800 last:border-0 pb-2 last:pb-0"
                >
                  <div className="flex items-center justify-between mb-1">
                    <Link
                      to={`/topics/${t.topic_name}`}
                      className="font-mono text-bagu-accent hover:underline"
                    >
                      {t.title || t.topic_name}
                    </Link>
                    <span className="text-xs text-slate-500">
                      {t.learned_cards}/{t.total_cards} · {(t.accuracy * 100).toFixed(0)}%
                      {t.due_today > 0 && (
                        <span className="ml-2 px-1.5 py-0.5 bg-amber-100 dark:bg-amber-900/40 text-amber-700 dark:text-amber-300 rounded text-[10px]">
                          今日 {t.due_today} 张
                        </span>
                      )}
                    </span>
                  </div>
                  <div className="h-1.5 bg-slate-200 dark:bg-slate-700 rounded-full overflow-hidden">
                    <div
                      className="h-full bg-bagu-accent transition-all"
                      style={{ width: `${pct}%` }}
                    />
                  </div>
                </li>
              );
            })}
          </ul>
        )}
      </section>

      {/* 薄弱卡片 */}
      <section className="border border-slate-200 dark:border-slate-700 rounded-lg p-4">
        <h2 className="text-base font-semibold mb-3 flex items-center gap-2">
          <AlertTriangle size={16} className="text-amber-600" />
          薄弱卡片（最近答错最多）
        </h2>
        {weak.loading && <p className="text-sm text-slate-500">加载中...</p>}
        {weak.error && (
          <p className="text-sm text-rose-600">{weak.error.message}</p>
        )}
        {weak.data && weak.data.items.length === 0 && (
          <p className="text-sm text-slate-500">没有薄弱卡片，太棒了！</p>
        )}
        {weak.data && weak.data.items.length > 0 && (
          <ol className="space-y-1.5 text-sm">
            {weak.data.items.map((w, i) => (
              <li key={w.card_id} className="flex items-start gap-3">
                <span className="text-xs text-slate-400 font-mono w-5 flex-shrink-0">
                  {i + 1}.
                </span>
                <div className="flex-1 min-w-0">
                  <Link
                    to={`/cards/${w.card_id}`}
                    className="hover:text-bagu-accent block truncate"
                  >
                    {w.question}
                  </Link>
                  <span className="text-xs text-slate-500">
                    {w.topic_name} · 错 {w.wrong_count}/{w.total_recent} 次
                  </span>
                </div>
              </li>
            ))}
          </ol>
        )}
      </section>
    </div>
  );
}

function StatCard({
  icon, label, value, hint,
}: { icon: React.ReactNode; label: string; value: string; hint?: string }) {
  return (
    <div className="border border-slate-200 dark:border-slate-700 rounded-lg p-3">
      <div className="text-xs text-slate-500 flex items-center gap-1.5">
        {icon} {label}
      </div>
      <div className="text-2xl font-mono font-bold mt-1">{value}</div>
      {hint && <div className="text-xs text-slate-400">{hint}</div>}
    </div>
  );
}
