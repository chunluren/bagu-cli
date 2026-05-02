import { Link, useParams } from 'react-router-dom';
import { ArrowLeft, Hash, FileText } from 'lucide-react';

import { api } from '../api/client';
import type { Card } from '../api/types';
import { useApi } from '../hooks/useApi';
import { MarkdownRenderer } from '../components/MarkdownRenderer';

export function CardPage() {
  const { id = '' } = useParams();
  const { data, error, loading } = useApi<Card>(
    () => api.get(`/api/cards/${encodeURIComponent(id)}`),
    [id]
  );

  if (loading) return <p className="text-slate-500">加载中...</p>;
  if (error) return <p className="text-rose-600">{error.message}</p>;
  if (!data) return null;

  return (
    <div>
      <button
        onClick={() => window.history.back()}
        className="text-sm text-slate-500 hover:text-bagu-accent flex items-center gap-1 mb-4"
      >
        <ArrowLeft size={14} /> 返回
      </button>

      <div className="flex items-center gap-3 text-xs text-slate-500 mb-2">
        <span className="flex items-center gap-1"><Hash size={12} />{data.id}</span>
        <Link to={`/topics/${data.topic_id}`} className="hover:text-bagu-accent">
          topic#{data.topic_id}
        </Link>
        <span className="px-1.5 py-0.5 rounded bg-slate-100 dark:bg-slate-800">
          {data.card_type}
        </span>
        <span className="flex items-center gap-1"><FileText size={12} />line {data.source_line}</span>
      </div>

      <h1 className="text-xl font-semibold mb-4 leading-snug">{data.question}</h1>

      <div className="border-t border-slate-200 dark:border-slate-700 pt-4">
        <MarkdownRenderer text={data.answer} />
      </div>
    </div>
  );
}
