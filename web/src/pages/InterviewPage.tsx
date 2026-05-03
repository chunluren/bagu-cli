import { useEffect, useRef, useState } from 'react';
import { Link, useSearchParams } from 'react-router-dom';
import { Bot, Send, Loader2, AlertCircle, Sparkles, RotateCcw } from 'lucide-react';

import { api, HttpError, streamSSE } from '../api/client';
import type {
  CreateSessionResponse,
  FinishSessionResponse,
  StreamEvent,
  Topic,
} from '../api/types';
import { useApi } from '../hooks/useApi';
import { MarkdownRenderer } from '../components/MarkdownRenderer';

type Phase =
  | { kind: 'setup' }
  | { kind: 'starting' }
  | { kind: 'asking'; question_no: number; partial: string }
  | { kind: 'answering'; question_no: number; question: string; askedAt: number }
  | { kind: 'grading'; question_no: number; question: string; answer: string; partial: string }
  | { kind: 'graded'; question_no: number; question: string; answer: string; feedback: string; score: number }
  | { kind: 'finished'; result: FinishSessionResponse }
  | { kind: 'error'; message: string };

interface Turn {
  question_no: number;
  question: string;
  answer: string;
  feedback: string;
  score: number;
}

export function InterviewPage() {
  const [params, setParams] = useSearchParams();
  const initialTopic = params.get('topic') || '';
  const initialNum = parseInt(params.get('num') || '5', 10) || 5;

  const topicsR = useApi<Topic[]>(() => api.get('/api/topics'), []);

  const [topic, setTopic] = useState(initialTopic);
  const [num, setNum] = useState(initialNum);
  const [provider, setProvider] = useState('');
  const [model, setModel] = useState('');

  const [session, setSession] = useState<CreateSessionResponse | null>(null);
  const [phase, setPhase] = useState<Phase>({ kind: 'setup' });
  const [history, setHistory] = useState<Turn[]>([]);
  const [pendingAnswer, setPendingAnswer] = useState('');

  const abortRef = useRef<AbortController | null>(null);

  // 同步 URL 参数（方便分享 / 刷新恢复 setup）
  useEffect(() => {
    const next = new URLSearchParams();
    if (topic) next.set('topic', topic);
    if (num !== 5) next.set('num', String(num));
    setParams(next, { replace: true });
  }, [topic, num, setParams]);

  // 卸载时取消进行中的流
  useEffect(() => {
    return () => abortRef.current?.abort();
  }, []);

  const askNext = async (sid: number, qno: number) => {
    setPhase({ kind: 'asking', question_no: qno, partial: '' });
    abortRef.current = new AbortController();
    try {
      let buffer = '';
      let finalQuestion = '';
      await streamSSE(
        `/api/interview/sessions/${sid}/question`,
        { method: 'GET' },
        (e: StreamEvent) => {
          if (e.type === 'chunk') {
            buffer += e.text;
            setPhase({ kind: 'asking', question_no: qno, partial: buffer });
          } else if (e.type === 'done') {
            finalQuestion = e.content || buffer;
          } else if (e.type === 'error') {
            setPhase({ kind: 'error', message: e.message });
          }
        },
        abortRef.current.signal
      );
      if (finalQuestion) {
        setPhase({
          kind: 'answering',
          question_no: qno,
          question: finalQuestion,
          askedAt: Date.now(),
        });
      }
    } catch (e) {
      if ((e as Error).name === 'AbortError') return;
      const msg = e instanceof HttpError ? e.message : String(e);
      setPhase({ kind: 'error', message: msg });
    }
  };

  const start = async () => {
    if (!topic) return;
    setPhase({ kind: 'starting' });
    setHistory([]);
    try {
      const r = await api.post<CreateSessionResponse>('/api/interview/sessions', {
        topic,
        question_count: num,
        provider: provider || undefined,
        model: model || undefined,
      });
      setSession(r);
      await askNext(r.session_id, 1);
    } catch (e) {
      const msg = e instanceof HttpError ? e.message : String(e);
      setPhase({ kind: 'error', message: msg });
    }
  };

  const submitAnswer = async () => {
    if (phase.kind !== 'answering' || !session) return;
    if (!pendingAnswer.trim()) return;
    const answer = pendingAnswer.trim();
    const duration_ms = Date.now() - phase.askedAt;
    const qno = phase.question_no;
    const question = phase.question;

    setPendingAnswer('');
    setPhase({ kind: 'grading', question_no: qno, question, answer, partial: '' });
    abortRef.current = new AbortController();

    try {
      let buffer = '';
      let finalScore = 0;
      let finalFeedback = '';
      await streamSSE(
        `/api/interview/sessions/${session.session_id}/answer`,
        {
          method: 'POST',
          body: JSON.stringify({ answer, duration_ms, question }),
        },
        (e: StreamEvent) => {
          if (e.type === 'chunk') {
            buffer += e.text;
            setPhase({ kind: 'grading', question_no: qno, question, answer, partial: buffer });
          } else if (e.type === 'done') {
            finalScore = e.score ?? 0;
            finalFeedback = e.feedback || buffer;
          } else if (e.type === 'error') {
            setPhase({ kind: 'error', message: e.message });
          }
        },
        abortRef.current.signal
      );

      const turn: Turn = {
        question_no: qno,
        question,
        answer,
        feedback: finalFeedback,
        score: finalScore,
      };
      setHistory((h) => [...h, turn]);
      setPhase({ kind: 'graded', ...turn });
    } catch (e) {
      if ((e as Error).name === 'AbortError') return;
      const msg = e instanceof HttpError ? e.message : String(e);
      setPhase({ kind: 'error', message: msg });
    }
  };

  const continueNext = async () => {
    if (!session) return;
    const qno = (phase as any).question_no + 1;
    if (qno > session.question_count) {
      await finish();
      return;
    }
    await askNext(session.session_id, qno);
  };

  const finish = async () => {
    if (!session) return;
    try {
      const r = await api.post<FinishSessionResponse>(
        `/api/interview/sessions/${session.session_id}/finish`,
        {}
      );
      setPhase({ kind: 'finished', result: r });
    } catch (e) {
      const msg = e instanceof HttpError ? e.message : String(e);
      setPhase({ kind: 'error', message: msg });
    }
  };

  const reset = () => {
    abortRef.current?.abort();
    setSession(null);
    setHistory([]);
    setPhase({ kind: 'setup' });
    setPendingAnswer('');
  };

  // ===== Setup 表单 =====
  if (phase.kind === 'setup') {
    return (
      <div className="max-w-2xl mx-auto">
        <div className="text-center mb-6">
          <Bot size={36} className="mx-auto text-bagu-accent mb-2" />
          <h1 className="text-2xl font-semibold">AI 模拟面试</h1>
          <p className="text-slate-500 text-sm mt-1">
            选定主题，AI 会基于你导入的卡片出题、评分。
          </p>
        </div>

        <div className="space-y-4 border border-slate-200 dark:border-slate-700 rounded-lg p-5">
          <div>
            <label className="block text-sm font-medium mb-1">主题</label>
            {topicsR.loading ? (
              <p className="text-xs text-slate-500">加载主题...</p>
            ) : (
              <select
                value={topic}
                onChange={(e) => setTopic(e.target.value)}
                className="w-full border border-slate-300 dark:border-slate-600 rounded-md px-3 py-2 bg-white dark:bg-slate-800 text-sm"
              >
                <option value="">— 选择主题 —</option>
                {(topicsR.data || []).map((t) => (
                  <option key={t.id} value={t.name}>
                    {t.title || t.name} ({t.cards ?? 0} 卡片)
                  </option>
                ))}
              </select>
            )}
          </div>

          <div>
            <label className="block text-sm font-medium mb-1">题数</label>
            <input
              type="number"
              min={1}
              max={20}
              value={num}
              onChange={(e) => setNum(parseInt(e.target.value, 10) || 5)}
              className="w-24 border border-slate-300 dark:border-slate-600 rounded-md px-3 py-2 bg-white dark:bg-slate-800 text-sm"
            />
            <span className="text-xs text-slate-500 ml-2">1-20</span>
          </div>

          <details className="text-sm">
            <summary className="cursor-pointer text-slate-600 dark:text-slate-400 select-none">
              高级：覆盖默认 LLM
            </summary>
            <div className="mt-3 space-y-3 pl-2 border-l-2 border-slate-200 dark:border-slate-700">
              <div>
                <label className="block text-xs text-slate-500 mb-1">Provider</label>
                <input
                  value={provider}
                  onChange={(e) => setProvider(e.target.value)}
                  placeholder="（默认从 ~/.bagu/config.toml 读取）"
                  className="w-full border border-slate-300 dark:border-slate-600 rounded-md px-3 py-1.5 bg-white dark:bg-slate-800 text-xs font-mono"
                />
              </div>
              <div>
                <label className="block text-xs text-slate-500 mb-1">Model</label>
                <input
                  value={model}
                  onChange={(e) => setModel(e.target.value)}
                  placeholder="如 gpt-4o-mini / claude-sonnet-4-6"
                  className="w-full border border-slate-300 dark:border-slate-600 rounded-md px-3 py-1.5 bg-white dark:bg-slate-800 text-xs font-mono"
                />
              </div>
            </div>
          </details>

          <button
            onClick={start}
            disabled={!topic}
            className="w-full py-2.5 bg-bagu-accent text-white rounded-md hover:bg-sky-600 disabled:opacity-50 disabled:cursor-not-allowed flex items-center justify-center gap-2 text-sm"
          >
            <Sparkles size={16} />
            开始面试
          </button>
        </div>

        <p className="text-xs text-slate-500 mt-4">
          需要先在 <code className="px-1 bg-slate-100 dark:bg-slate-800 rounded">~/.bagu/config.toml</code> 配置
          LLM provider 和 API key 环境变量。
        </p>

        <div className="mt-6 text-center">
          <Link
            to="/interview/history"
            className="text-sm text-slate-500 hover:text-bagu-accent inline-flex items-center gap-1.5"
          >
            查看历史会话 →
          </Link>
        </div>
      </div>
    );
  }

  // ===== 进行中 / 已完成 =====
  return (
    <div className="max-w-3xl mx-auto">
      {/* 顶部进度 */}
      <div className="flex items-center justify-between text-xs text-slate-500 mb-4">
        <span>
          {session && (
            <>
              <span className="font-mono text-bagu-accent">{session.topic}</span>
              {' · '}{session.provider}/{session.model}
            </>
          )}
        </span>
        <button onClick={reset} className="hover:text-bagu-accent flex items-center gap-1">
          <RotateCcw size={12} /> 重新开始
        </button>
      </div>

      {/* 历史回合（折叠展示） */}
      {history.length > 0 && (
        <div className="mb-4 space-y-2">
          {history.map((t) => (
            <details
              key={t.question_no}
              className="border border-slate-200 dark:border-slate-700 rounded-md text-sm"
            >
              <summary className="px-3 py-2 cursor-pointer flex items-center justify-between select-none">
                <span>
                  Q{t.question_no} · {t.question.slice(0, 40)}
                  {t.question.length > 40 ? '…' : ''}
                </span>
                <span className={`font-mono text-xs ${
                  t.score >= 7 ? 'text-emerald-600' :
                  t.score >= 4 ? 'text-amber-600' : 'text-rose-600'
                }`}>
                  {t.score}/10
                </span>
              </summary>
              <div className="px-4 pb-3 pt-1 text-xs space-y-2 border-t border-slate-200 dark:border-slate-700">
                <div>
                  <div className="text-slate-400 mb-1">题目</div>
                  <MarkdownRenderer text={t.question} />
                </div>
                <div>
                  <div className="text-slate-400 mb-1">你的回答</div>
                  <pre className="whitespace-pre-wrap font-sans">{t.answer}</pre>
                </div>
                <div>
                  <div className="text-slate-400 mb-1">评分</div>
                  <MarkdownRenderer text={t.feedback} />
                </div>
              </div>
            </details>
          ))}
        </div>
      )}

      {/* 出题中 */}
      {(phase.kind === 'starting' || phase.kind === 'asking') && (
        <div className="border border-slate-200 dark:border-slate-700 rounded-lg p-5">
          <div className="text-xs text-slate-400 mb-2 flex items-center gap-2">
            <Loader2 size={14} className="animate-spin" />
            {phase.kind === 'starting' ? '初始化会话…' : `第 ${phase.question_no} 题 · 出题中…`}
          </div>
          {phase.kind === 'asking' && phase.partial && (
            <MarkdownRenderer text={phase.partial} />
          )}
        </div>
      )}

      {/* 答题 */}
      {phase.kind === 'answering' && (
        <div className="space-y-4">
          <div className="border border-slate-200 dark:border-slate-700 rounded-lg p-5">
            <div className="text-xs text-slate-400 mb-2">
              第 {phase.question_no} / {session?.question_count} 题
            </div>
            <MarkdownRenderer text={phase.question} />
          </div>

          <div className="border border-slate-200 dark:border-slate-700 rounded-lg p-3">
            <textarea
              value={pendingAnswer}
              onChange={(e) => setPendingAnswer(e.target.value)}
              placeholder="输入你的答案... (Cmd/Ctrl + Enter 提交)"
              rows={6}
              className="w-full bg-transparent outline-none text-sm resize-y font-sans"
              onKeyDown={(e) => {
                if ((e.metaKey || e.ctrlKey) && e.key === 'Enter') {
                  e.preventDefault();
                  submitAnswer();
                }
              }}
              autoFocus
            />
            <div className="flex justify-between items-center mt-2 pt-2 border-t border-slate-200 dark:border-slate-700">
              <span className="text-xs text-slate-400">{pendingAnswer.length} 字</span>
              <button
                onClick={submitAnswer}
                disabled={!pendingAnswer.trim()}
                className="px-4 py-1.5 bg-bagu-accent text-white rounded-md hover:bg-sky-600 disabled:opacity-50 flex items-center gap-1.5 text-sm"
              >
                <Send size={14} /> 提交
              </button>
            </div>
          </div>
        </div>
      )}

      {/* 评分中 */}
      {phase.kind === 'grading' && (
        <div className="space-y-4">
          <div className="border border-slate-200 dark:border-slate-700 rounded-lg p-5">
            <div className="text-xs text-slate-400 mb-2">第 {phase.question_no} 题</div>
            <MarkdownRenderer text={phase.question} />
          </div>
          <div className="border border-slate-200 dark:border-slate-700 rounded-lg p-3 text-sm bg-slate-50 dark:bg-slate-800/40">
            <div className="text-xs text-slate-400 mb-1">你的回答</div>
            <pre className="whitespace-pre-wrap font-sans text-xs">{phase.answer}</pre>
          </div>
          <div className="border border-slate-200 dark:border-slate-700 rounded-lg p-5">
            <div className="text-xs text-slate-400 mb-2 flex items-center gap-2">
              <Loader2 size={14} className="animate-spin" />
              评分中…
            </div>
            {phase.partial && <MarkdownRenderer text={phase.partial} />}
          </div>
        </div>
      )}

      {/* 评分完成 */}
      {phase.kind === 'graded' && (
        <div className="space-y-4">
          <div className="border border-slate-200 dark:border-slate-700 rounded-lg p-5">
            <div className="flex items-center justify-between mb-2">
              <div className="text-xs text-slate-400">
                第 {phase.question_no} 题 · 评分
              </div>
              <div className={`font-mono text-lg font-bold ${
                phase.score >= 7 ? 'text-emerald-600' :
                phase.score >= 4 ? 'text-amber-600' : 'text-rose-600'
              }`}>
                {phase.score}/10
              </div>
            </div>
            <MarkdownRenderer text={phase.feedback} />
          </div>
          <div className="flex justify-end gap-2">
            {session && phase.question_no >= session.question_count ? (
              <button
                onClick={finish}
                className="px-4 py-2 bg-bagu-accent text-white rounded-md hover:bg-sky-600 text-sm"
              >
                查看总结
              </button>
            ) : (
              <button
                onClick={continueNext}
                className="px-4 py-2 bg-bagu-accent text-white rounded-md hover:bg-sky-600 text-sm"
              >
                下一题 →
              </button>
            )}
          </div>
        </div>
      )}

      {/* 完成 */}
      {phase.kind === 'finished' && (
        <div className="text-center py-12">
          <div className="text-5xl mb-4">🎓</div>
          <h1 className="text-2xl font-semibold mb-3">面试完成！</h1>
          <div className="inline-grid grid-cols-3 gap-6 text-sm bg-slate-50 dark:bg-slate-800 rounded-lg p-6 mb-6">
            <div>
              <div className="text-2xl font-mono font-bold">{phase.result.answered}</div>
              <div className="text-slate-500">答题数</div>
            </div>
            <div>
              <div className="text-2xl font-mono font-bold text-emerald-600">
                {phase.result.avg_score.toFixed(1)}
              </div>
              <div className="text-slate-500">平均分</div>
            </div>
            <div>
              <div className="text-2xl font-mono font-bold text-bagu-accent">
                {phase.result.total_score}
              </div>
              <div className="text-slate-500">总分</div>
            </div>
          </div>
          <div className="flex justify-center gap-2">
            <button
              onClick={reset}
              className="px-4 py-2 bg-bagu-accent text-white rounded-md hover:bg-sky-600 text-sm"
            >
              再来一轮
            </button>
            <Link
              to="/"
              className="px-4 py-2 border border-slate-300 dark:border-slate-600 rounded-md hover:bg-slate-100 dark:hover:bg-slate-800 text-sm"
            >
              返回主页
            </Link>
          </div>
        </div>
      )}

      {/* 错误 */}
      {phase.kind === 'error' && (
        <div className="border border-rose-300 dark:border-rose-700 bg-rose-50 dark:bg-rose-900/20 rounded-lg p-5">
          <div className="flex items-start gap-2">
            <AlertCircle size={18} className="text-rose-600 flex-shrink-0 mt-0.5" />
            <div className="flex-1">
              <div className="font-semibold text-rose-700 dark:text-rose-300 mb-1">
                出错了
              </div>
              <p className="text-sm text-rose-600 dark:text-rose-400">{phase.message}</p>
              <button
                onClick={reset}
                className="mt-3 px-3 py-1.5 text-xs border border-rose-300 dark:border-rose-700 rounded hover:bg-rose-100 dark:hover:bg-rose-900/30"
              >
                返回设置
              </button>
            </div>
          </div>
        </div>
      )}
    </div>
  );
}
