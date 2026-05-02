import { useEffect } from 'react';

interface Props {
  onScore: (score: number) => void;
  disabled?: boolean;
}

const SCORES = [
  { value: 0, label: '完全忘', color: 'bg-rose-600 hover:bg-rose-700' },
  { value: 1, label: '答错', color: 'bg-rose-500 hover:bg-rose-600' },
  { value: 2, label: '印象', color: 'bg-orange-500 hover:bg-orange-600' },
  { value: 3, label: '答对(难)', color: 'bg-amber-500 hover:bg-amber-600' },
  { value: 4, label: '答对', color: 'bg-emerald-500 hover:bg-emerald-600' },
  { value: 5, label: '完美', color: 'bg-emerald-600 hover:bg-emerald-700' },
];

export function ScoreButtons({ onScore, disabled }: Props) {
  useEffect(() => {
    if (disabled) return;
    const handler = (e: KeyboardEvent) => {
      const k = e.key;
      if (k >= '0' && k <= '5') {
        e.preventDefault();
        onScore(Number(k));
      }
    };
    window.addEventListener('keydown', handler);
    return () => window.removeEventListener('keydown', handler);
  }, [onScore, disabled]);

  return (
    <div className="grid grid-cols-3 sm:grid-cols-6 gap-2">
      {SCORES.map((s) => (
        <button
          key={s.value}
          onClick={() => !disabled && onScore(s.value)}
          disabled={disabled}
          className={`py-3 sm:py-2 rounded-md text-white font-medium text-sm transition disabled:opacity-50 ${s.color}`}
        >
          <div className="text-base font-mono">{s.value}</div>
          <div className="text-xs opacity-90">{s.label}</div>
        </button>
      ))}
    </div>
  );
}
