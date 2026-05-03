import { Link } from 'react-router-dom';
import { History, Bot } from 'lucide-react';

import { api } from '../api/client';
import type { InterviewSession } from '../api/types';
import { useApi } from '../hooks/useApi';

function formatTimestamp(ts: number): string {
  if (!ts) return '-';
  const d = new Date(ts * 1000);
  // YYYY-MM-DD HH:mm
  const pad = (n: number) => String(n).padStart(2, '0');
  return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())} ${pad(d.getHours())}:${pad(d.getMinutes())}`;
}

function durationMin(start: number, end: number): string {
  if (!start || !end || end < start) return '-';
  const sec = end - start;
  if (sec < 60) return `${sec}s`;
  return `${Math.round(sec / 60)}min`;
}

export function InterviewHistoryPage() {
  const { data, error, loading } = useApi<{ items: InterviewSession[] }>(
    () => api.get('/api/interview/sessions?limit=50'),
    []
  );

  return (
    <div className="max-w-3xl mx-auto">
      <div className="flex items-center justify-between mb-6">
        <h1 className="text-2xl font-semibold flex items-center gap-2">
          <History size={24} className="text-bagu-accent" />
          面试历史
        </h1>
        <Link
          to="/interview"
          className="px-3 py-1.5 bg-bagu-accent text-white rounded-md hover:bg-sky-600 text-sm flex items-center gap-1.5"
        >
          <Bot size={14} /> 新会话
        </Link>
      </div>

      {loading && <p className="text-sm text-slate-500">加载中...</p>}
      {error && <p className="text-sm text-rose-600">{error.message}</p>}

      {data && data.items.length === 0 && (
        <div className="text-center py-12 border border-dashed border-slate-300 dark:border-slate-700 rounded-lg">
          <Bot size={32} className="mx-auto text-slate-400 mb-2" />
          <p className="text-sm text-slate-500">还没有面试会话。</p>
          <Link
            to="/interview"
            className="inline-block mt-3 text-sm text-bagu-accent hover:underline"
          >
            开始第一次模拟面试 →
          </Link>
        </div>
      )}

      {data && data.items.length > 0 && (
        <div className="border border-slate-200 dark:border-slate-700 rounded-lg overflow-hidden">
          <table className="w-full text-sm">
            <thead className="bg-slate-50 dark:bg-slate-800/40 text-xs text-slate-500">
              <tr>
                <th className="text-left px-3 py-2">主题</th>
                <th className="text-left px-3 py-2">开始时间</th>
                <th className="text-right px-3 py-2">题数</th>
                <th className="text-right px-3 py-2">平均分</th>
                <th className="text-right px-3 py-2">用时</th>
                <th className="text-left px-3 py-2">模型</th>
              </tr>
            </thead>
            <tbody>
              {data.items.map((s) => {
                const finished = s.ended_at > 0;
                const avg = finished ? s.total_score : null;
                return (
                  <tr
                    key={s.id}
                    className="border-t border-slate-100 dark:border-slate-800 hover:bg-slate-50 dark:hover:bg-slate-800/40"
                  >
                    <td className="px-3 py-2">
                      <Link
                        to={`/interview/sessions/${s.id}`}
                        className="font-mono text-bagu-accent hover:underline"
                      >
                        {s.topic}
                      </Link>
                      {!finished && (
                        <span className="ml-2 px-1.5 py-0.5 bg-amber-100 dark:bg-amber-900/40 text-amber-700 dark:text-amber-300 rounded text-[10px]">
                          未完成
                        </span>
                      )}
                    </td>
                    <td className="px-3 py-2 text-slate-500 text-xs">
                      {formatTimestamp(s.started_at)}
                    </td>
                    <td className="px-3 py-2 text-right font-mono text-xs">
                      {s.question_count || '-'}
                    </td>
                    <td className="px-3 py-2 text-right">
                      {avg !== null ? (
                        <span className={`font-mono text-xs font-semibold ${
                          avg >= 7 ? 'text-emerald-600' :
                          avg >= 4 ? 'text-amber-600' : 'text-rose-600'
                        }`}>
                          {avg.toFixed(1)}/10
                        </span>
                      ) : (
                        <span className="text-slate-400 text-xs">-</span>
                      )}
                    </td>
                    <td className="px-3 py-2 text-right text-slate-500 text-xs">
                      {durationMin(s.started_at, s.ended_at)}
                    </td>
                    <td className="px-3 py-2 text-slate-500 text-xs font-mono">
                      {s.llm_provider}/{s.llm_model || '-'}
                    </td>
                  </tr>
                );
              })}
            </tbody>
          </table>
        </div>
      )}
    </div>
  );
}
