import { useMemo } from 'react';
import { useParams, Link } from 'react-router-dom';
import { ChevronRight } from 'lucide-react';

import { api } from '../api/client';
import type { Card, Chapter, TopicDetail } from '../api/types';
import { useApi } from '../hooks/useApi';

export function TopicPage() {
  const { name = '' } = useParams();

  const topicState = useApi<TopicDetail>(
    () => api.get(`/api/topics/${encodeURIComponent(name)}`),
    [name]
  );

  const cardsState = useApi<{ topic: string; items: Card[]; total: number }>(
    () => api.get(`/api/cards?topic=${encodeURIComponent(name)}`),
    [name]
  );

  // 章节按顶级（chapter_no > 0 升序，负数排末尾）+ 子章节分组
  const tree = useMemo(() => {
    const chapters = topicState.data?.chapters || [];
    const tops = chapters
      .filter((c) => c.parent_id == null)
      .sort((a, b) => {
        const an = a.chapter_no < 0;
        const bn = b.chapter_no < 0;
        if (an !== bn) return an ? 1 : -1;
        return a.chapter_no - b.chapter_no;
      });
    const byParent: Record<number, Chapter[]> = {};
    chapters.forEach((c) => {
      if (c.parent_id != null) {
        (byParent[c.parent_id] ||= []).push(c);
      }
    });
    Object.values(byParent).forEach((arr) =>
      arr.sort((a, b) => a.chapter_no - b.chapter_no)
    );
    return { tops, byParent };
  }, [topicState.data]);

  const cardsByChapter = useMemo(() => {
    const map: Record<number, Card[]> = {};
    cardsState.data?.items.forEach((c) => {
      if (c.chapter_id != null) (map[c.chapter_id] ||= []).push(c);
    });
    return map;
  }, [cardsState.data]);

  if (topicState.loading) return <p className="text-slate-500">加载中...</p>;
  if (topicState.error)
    return <p className="text-rose-600">{topicState.error.message}</p>;
  if (!topicState.data) return null;

  const t = topicState.data;

  return (
    <div>
      <div className="mb-4">
        <Link to="/" className="text-sm text-slate-500 hover:text-bagu-accent">
          ← 返回主题列表
        </Link>
        <h1 className="text-2xl font-semibold mt-1">{t.title}</h1>
        <div className="text-sm text-slate-500 mt-1">
          {cardsState.data?.total ?? '—'} cards · {tree.tops.length} chapters
        </div>
      </div>

      <ul className="space-y-1">
        {tree.tops.map((ch) => {
          const subs = tree.byParent[ch.id] || [];
          const directCards = cardsByChapter[ch.id] || [];
          return (
            <li key={ch.id}>
              <div className="font-medium py-1">
                {ch.chapter_no > 0 ? `第 ${ch.chapter_no} 章 ` : ''}
                {ch.name}
                {directCards.length > 0 && (
                  <span className="text-xs text-slate-500 ml-2">
                    ({directCards.length} cards)
                  </span>
                )}
              </div>
              {subs.length > 0 && (
                <ul className="ml-4 border-l border-slate-200 dark:border-slate-700 pl-3 space-y-0.5">
                  {subs.map((sub) => {
                    const subCards = cardsByChapter[sub.id] || [];
                    return (
                      <li key={sub.id}>
                        <div className="text-sm py-0.5">{sub.name}</div>
                        {subCards.length > 0 && (
                          <ul className="ml-3 space-y-0.5">
                            {subCards.map((card) => (
                              <li key={card.id}>
                                <Link
                                  to={`/cards/${card.id}`}
                                  className="flex items-center gap-1 text-xs text-slate-500 hover:text-bagu-accent py-0.5"
                                >
                                  <ChevronRight size={10} />
                                  <span className="truncate">{card.question}</span>
                                </Link>
                              </li>
                            ))}
                          </ul>
                        )}
                      </li>
                    );
                  })}
                </ul>
              )}
              {/* 顶级章节直接含的卡片（无子章节归属）*/}
              {directCards.length > 0 && subs.length === 0 && (
                <ul className="ml-4 border-l border-slate-200 dark:border-slate-700 pl-3 space-y-0.5">
                  {directCards.map((card) => (
                    <li key={card.id}>
                      <Link
                        to={`/cards/${card.id}`}
                        className="flex items-center gap-1 text-xs text-slate-500 hover:text-bagu-accent py-0.5"
                      >
                        <ChevronRight size={10} />
                        <span className="truncate">{card.question}</span>
                      </Link>
                    </li>
                  ))}
                </ul>
              )}
            </li>
          );
        })}
      </ul>
    </div>
  );
}
