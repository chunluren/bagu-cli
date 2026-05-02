import { useMemo } from 'react';
import { Link } from 'react-router-dom';

import type { DailyCount } from '../api/types';

interface Props {
  /// 按日期升序，最后一项 = 今天
  items: DailyCount[];
  /// 点击空格的链接生成器（默认：复习页带 max_new=20）
  hrefForDate?: (date: string) => string;
}

const COLORS = [
  'bg-slate-100 dark:bg-slate-800',                // 0
  'bg-emerald-200 dark:bg-emerald-900',            // 1-2
  'bg-emerald-400 dark:bg-emerald-700',            // 3-9
  'bg-emerald-500 dark:bg-emerald-500',            // 10-19
  'bg-emerald-600 dark:bg-emerald-400',            // 20+
];

function bucket(n: number): number {
  if (n === 0) return 0;
  if (n < 3) return 1;
  if (n < 10) return 2;
  if (n < 20) return 3;
  return 4;
}

/// GitHub 风格热力图：按周排列（每列 7 天，从周一开始）
export function Heatmap({ items, hrefForDate }: Props) {
  // 把 items 切成 7×N 的方阵
  const grid = useMemo(() => {
    if (items.length === 0) return { cols: [] as DailyCount[][], total: 0, max: 0 };

    // 用第一项的星期几对齐起点（确保每列 = 一周）
    const first = new Date(items[0].date + 'T00:00:00');
    const firstWeekday = (first.getDay() + 6) % 7;  // 周一=0
    const padding: (DailyCount | null)[] = Array(firstWeekday).fill(null);
    const cells: (DailyCount | null)[] = [...padding, ...items];

    const cols: (DailyCount | null)[][] = [];
    for (let i = 0; i < cells.length; i += 7) {
      cols.push(cells.slice(i, i + 7));
    }
    let total = 0;
    let max = 0;
    for (const it of items) {
      total += it.count;
      if (it.count > max) max = it.count;
    }
    return { cols, total, max };
  }, [items]);

  if (items.length === 0) {
    return <p className="text-sm text-slate-500">还没有复习记录。</p>;
  }

  return (
    <div>
      <div className="flex items-center gap-3 text-xs text-slate-500 mb-2">
        <span>{items.length} 天 · 共 {grid.total} 次复习 · 单日峰值 {grid.max}</span>
      </div>
      <div className="overflow-x-auto pb-2">
        <div className="inline-flex gap-[3px]">
          {grid.cols.map((week, ci) => (
            <div key={ci} className="flex flex-col gap-[3px]">
              {week.map((cell, ri) => {
                if (!cell) {
                  return <div key={ri} className="w-3 h-3" />;
                }
                const cls = COLORS[bucket(cell.count)];
                const tip = `${cell.date} · ${cell.count} 次`;
                const href = hrefForDate?.(cell.date);
                if (href) {
                  return (
                    <Link
                      key={ri}
                      to={href}
                      title={tip}
                      className={`w-3 h-3 rounded-sm ${cls} hover:ring-1 hover:ring-bagu-accent`}
                    />
                  );
                }
                return (
                  <div
                    key={ri}
                    title={tip}
                    className={`w-3 h-3 rounded-sm ${cls}`}
                  />
                );
              })}
            </div>
          ))}
        </div>
      </div>
      <div className="flex items-center gap-1.5 text-xs text-slate-500 mt-2">
        <span>少</span>
        {COLORS.map((c, i) => (
          <span key={i} className={`w-3 h-3 rounded-sm ${c}`} />
        ))}
        <span>多</span>
      </div>
    </div>
  );
}
