import { useEffect, useMemo, useState } from 'react';
import { Link, useSearchParams } from 'react-router-dom';
import { Brain, Check, X as XIcon } from 'lucide-react';

import { api, HttpError } from '../api/client';
import type { DueCard, DueResponse, ReviewState } from '../api/types';
import { useApi } from '../hooks/useApi';
import { MarkdownRenderer } from '../components/MarkdownRenderer';
import { ScoreButtons } from '../components/ScoreButtons';

interface SessionStats {
  reviewed: number;
  correct: number;
  total: number;
  results: { card_id: number; score: number }[];
}

export function ReviewPage() {
  const [params] = useSearchParams();
  const topic = params.get('topic') || '';
  const maxNew = params.get('max_new') ?? '5';
  const maxDue = params.get('max_due') ?? '20';

  const url = useMemo(() => {
    const q = new URLSearchParams();
    if (topic) q.set('topic', topic);
    q.set('max_new', maxNew);
    q.set('max_due', maxDue);
    return `/api/review/due?${q.toString()}`;
  }, [topic, maxNew, maxDue]);

  const { data, error, loading } = useApi<DueResponse>(() => api.get(url), [url]);

  const [idx, setIdx] = useState(0);
  const [revealed, setRevealed] = useState(false);
  const [submitting, setSubmitting] = useState(false);
  const [stats, setStats] = useState<SessionStats>({
    reviewed: 0,
    correct: 0,
    total: 0,
    results: [],
  });
  const [lastResult, setLastResult] = useState<ReviewState | null>(null);
  const [cardStartTime, setCardStartTime] = useState<number>(Date.now());

  // 初始化总数
  useEffect(() => {
    if (data) {
      setStats({ reviewed: 0, correct: 0, total: data.cards.length, results: [] });
      setIdx(0);
      setRevealed(false);
      setCardStartTime(Date.now());
    }
  }, [data]);

  // 空格 / Enter 揭示答案
  useEffect(() => {
    if (revealed) return;
    const h = (e: KeyboardEvent) => {
      if (e.key === ' ' || e.key === 'Enter') {
        e.preventDefault();
        setRevealed(true);
      }
    };
    window.addEventListener('keydown', h);
    return () => window.removeEventListener('keydown', h);
  }, [revealed]);

  if (loading) return <p className="text-slate-500">加载复习卡片...</p>;
  if (error) return <p className="text-rose-600">{error.message}</p>;

  const cards = data?.cards || [];

  if (cards.length === 0) {
    return (
      <div className="text-center py-12">
        <Brain size={32} className="mx-auto text-bagu-accent mb-3" />
        <h1 className="text-xl font-semibold mb-2">🎉 今天没有要复习的卡片！</h1>
        <p className="text-slate-500 text-sm">
          {topic ? `主题 ${topic} 的所有卡片都已掌握或未到期。` : '所有卡片都已掌握或未到期。'}
        </p>
        <Link
          to={`/review${topic ? `?topic=${topic}&max_new=20` : '?max_new=20'}`}
          className="inline-block mt-4 text-bagu-accent hover:underline text-sm"
        >
          学新卡 →
        </Link>
      </div>
    );
  }

  // 完成
  if (idx >= cards.length) {
    const acc = stats.reviewed > 0
      ? Math.round((stats.correct / stats.reviewed) * 100)
      : 0;
    return (
      <div className="text-center py-12">
        <div className="text-5xl mb-4">🎉</div>
        <h1 className="text-2xl font-semibold mb-3">本轮复习完成！</h1>
        <div className="inline-grid grid-cols-3 gap-6 text-sm bg-slate-50 dark:bg-slate-800 rounded-lg p-6 mb-6">
          <div>
            <div className="text-2xl font-mono font-bold">{stats.reviewed}</div>
            <div className="text-slate-500">复习</div>
          </div>
          <div>
            <div className="text-2xl font-mono font-bold text-emerald-600">
              {stats.correct}
            </div>
            <div className="text-slate-500">答对</div>
          </div>
          <div>
            <div className="text-2xl font-mono font-bold text-bagu-accent">
              {acc}%
            </div>
            <div className="text-slate-500">正确率</div>
          </div>
        </div>
        <div className="flex justify-center gap-2">
          <button
            onClick={() => window.location.reload()}
            className="px-4 py-2 bg-bagu-accent text-white rounded-md hover:bg-sky-600"
          >
            再来一轮
          </button>
          <Link
            to="/"
            className="px-4 py-2 border border-slate-300 dark:border-slate-600 rounded-md hover:bg-slate-100 dark:hover:bg-slate-800"
          >
            返回主页
          </Link>
        </div>
      </div>
    );
  }

  const card: DueCard = cards[idx];

  const submit = async (score: number) => {
    if (submitting) return;
    setSubmitting(true);
    const duration_ms = Date.now() - cardStartTime;
    try {
      const r = await api.post<ReviewState>(
        `/api/review/${card.card_id}/grade`,
        { score, duration_ms }
      );
      setLastResult(r);
      setStats((s) => ({
        reviewed: s.reviewed + 1,
        correct: s.correct + (score >= 3 ? 1 : 0),
        total: s.total,
        results: [...s.results, { card_id: card.card_id, score }],
      }));
      // 短暂显示反馈再切下一题
      setTimeout(() => {
        setIdx((i) => i + 1);
        setRevealed(false);
        setLastResult(null);
        setSubmitting(false);
        setCardStartTime(Date.now());
      }, 500);
    } catch (e) {
      const msg = e instanceof HttpError ? e.message : String(e);
      alert('提交失败：' + msg);
      setSubmitting(false);
    }
  };

  const skip = () => {
    setIdx((i) => i + 1);
    setRevealed(false);
    setCardStartTime(Date.now());
  };

  const progress = ((idx) / cards.length) * 100;

  return (
    <div className="max-w-3xl mx-auto">
      {/* 顶部：进度 */}
      <div className="flex items-center justify-between text-xs text-slate-500 mb-2">
        <span>
          {idx + 1} / {cards.length} ·{' '}
          <span className="font-mono text-bagu-accent">{card.topic_name}</span>
          {card.is_new && (
            <span className="ml-2 px-1.5 py-0.5 bg-amber-100 dark:bg-amber-900/40 text-amber-700 dark:text-amber-300 rounded text-[10px]">
              NEW
            </span>
          )}
        </span>
        <span>已答对 {stats.correct}/{stats.reviewed}</span>
      </div>
      <div className="h-1.5 bg-slate-200 dark:bg-slate-700 rounded-full overflow-hidden mb-6">
        <div
          className="h-full bg-bagu-accent transition-all"
          style={{ width: `${progress}%` }}
        />
      </div>

      {/* 卡片 */}
      <div className="border border-slate-200 dark:border-slate-700 rounded-lg p-5 sm:p-6">
        <div className="text-xs text-slate-400 mb-2">Q · card#{card.card_id}</div>
        <h2 className="text-xl font-semibold mb-4 leading-snug">{card.question}</h2>

        {!revealed ? (
          <div className="text-center py-10">
            <button
              onClick={() => setRevealed(true)}
              className="px-6 py-3 bg-slate-100 dark:bg-slate-800 hover:bg-slate-200 dark:hover:bg-slate-700 rounded-md text-sm transition"
            >
              点击或按 <kbd className="bg-white dark:bg-slate-900 px-1.5 py-0.5 rounded border text-xs">SPACE</kbd> 显示答案
            </button>
          </div>
        ) : (
          <>
            <div className="border-t border-slate-200 dark:border-slate-700 pt-4 mb-6">
              <div className="text-xs text-slate-400 mb-2">A</div>
              <MarkdownRenderer text={card.answer} />
            </div>

            {lastResult && (
              <div className="text-center mb-3 text-sm flex items-center justify-center gap-2">
                {stats.results[stats.results.length - 1]?.score >= 3 ? (
                  <Check size={16} className="text-emerald-600" />
                ) : (
                  <XIcon size={16} className="text-rose-600" />
                )}
                <span className="text-slate-600 dark:text-slate-400">
                  下次复习: {lastResult.interval_days} 天后 · ease {lastResult.ease_factor.toFixed(2)}
                </span>
              </div>
            )}

            <ScoreButtons onScore={submit} disabled={submitting} />
          </>
        )}
      </div>

      {/* 底部辅助按钮 */}
      <div className="flex justify-center gap-2 mt-4 text-xs">
        <button
          onClick={skip}
          className="px-3 py-1.5 text-slate-500 hover:text-bagu-accent"
        >
          跳过此题 (s)
        </button>
        <Link to="/" className="px-3 py-1.5 text-slate-500 hover:text-bagu-accent">
          退出会话 (q)
        </Link>
      </div>
    </div>
  );
}
