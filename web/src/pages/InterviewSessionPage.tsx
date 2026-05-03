import { Link, useParams } from 'react-router-dom';
import { ArrowLeft, Clock, Award } from 'lucide-react';

import { api } from '../api/client';
import type { InterviewSessionDetail } from '../api/types';
import { useApi } from '../hooks/useApi';
import { MarkdownRenderer } from '../components/MarkdownRenderer';

function formatTimestamp(ts: number): string {
  if (!ts) return '-';
  const d = new Date(ts * 1000);
  const pad = (n: number) => String(n).padStart(2, '0');
  return `${d.getFullYear()}-${pad(d.getMonth() + 1)}-${pad(d.getDate())} ${pad(d.getHours())}:${pad(d.getMinutes())}`;
}

export function InterviewSessionPage() {
  const { id } = useParams<{ id: string }>();
  const { data, error, loading } = useApi<InterviewSessionDetail>(
    () => api.get(`/api/interview/sessions/${id}`),
    [id]
  );

  return (
    <div className="max-w-3xl mx-auto">
      <Link
        to="/interview/history"
        className="inline-flex items-center gap-1 text-sm text-slate-500 hover:text-bagu-accent mb-4"
      >
        <ArrowLeft size={14} /> 返回历史
      </Link>

      {loading && <p className="text-sm text-slate-500">加载中...</p>}
      {error && <p className="text-sm text-rose-600">{error.message}</p>}

      {data && (
        <>
          <div className="mb-6 border border-slate-200 dark:border-slate-700 rounded-lg p-5">
            <div className="flex items-center justify-between mb-3">
              <h1 className="text-xl font-semibold">
                <span className="font-mono text-bagu-accent">{data.topic}</span>
                <span className="text-slate-400 text-sm ml-2">#{data.id}</span>
              </h1>
              {data.ended_at > 0 && (
                <div className={`font-mono text-2xl font-bold ${
                  data.total_score >= 7 ? 'text-emerald-600' :
                  data.total_score >= 4 ? 'text-amber-600' : 'text-rose-600'
                }`}>
                  {data.total_score.toFixed(1)}/10
                </div>
              )}
            </div>
            <div className="grid grid-cols-2 sm:grid-cols-4 gap-3 text-xs">
              <div>
                <div className="text-slate-400 flex items-center gap-1">
                  <Clock size={12} /> 开始
                </div>
                <div>{formatTimestamp(data.started_at)}</div>
              </div>
              <div>
                <div className="text-slate-400 flex items-center gap-1">
                  <Clock size={12} /> 结束
                </div>
                <div>{data.ended_at > 0 ? formatTimestamp(data.ended_at) : '未完成'}</div>
              </div>
              <div>
                <div className="text-slate-400 flex items-center gap-1">
                  <Award size={12} /> 题数
                </div>
                <div className="font-mono">{data.question_count}</div>
              </div>
              <div>
                <div className="text-slate-400">模型</div>
                <div className="font-mono">{data.llm_provider}/{data.llm_model || '-'}</div>
              </div>
            </div>
          </div>

          {data.qas.length === 0 ? (
            <p className="text-sm text-slate-500 text-center py-8">
              这次会话没有作答记录。
            </p>
          ) : (
            <div className="space-y-3">
              {data.qas.map((qa) => (
                <div
                  key={qa.id}
                  className="border border-slate-200 dark:border-slate-700 rounded-lg overflow-hidden"
                >
                  <div className="px-4 py-2 bg-slate-50 dark:bg-slate-800/40 flex items-center justify-between">
                    <span className="text-sm font-semibold">
                      Q{qa.question_no}
                    </span>
                    <span className={`font-mono text-sm ${
                      qa.ai_score >= 7 ? 'text-emerald-600' :
                      qa.ai_score >= 4 ? 'text-amber-600' : 'text-rose-600'
                    }`}>
                      {qa.ai_score}/10
                      <span className="text-xs text-slate-400 ml-2">
                        {qa.duration_ms > 0 ? `${Math.round(qa.duration_ms / 1000)}s` : ''}
                      </span>
                    </span>
                  </div>
                  <div className="p-4 space-y-3 text-sm">
                    <div>
                      <div className="text-xs text-slate-400 mb-1">题目</div>
                      <MarkdownRenderer text={qa.question} />
                    </div>
                    <div>
                      <div className="text-xs text-slate-400 mb-1">你的回答</div>
                      <pre className="whitespace-pre-wrap font-sans text-[13px] bg-slate-50 dark:bg-slate-800/40 p-2 rounded">
                        {qa.user_answer || <span className="text-slate-400">（未作答）</span>}
                      </pre>
                    </div>
                    <div>
                      <div className="text-xs text-slate-400 mb-1">AI 评分</div>
                      <MarkdownRenderer text={qa.ai_feedback} />
                    </div>
                  </div>
                </div>
              ))}
            </div>
          )}
        </>
      )}
    </div>
  );
}
