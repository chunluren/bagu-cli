import type { ApiError } from './types';

const BASE = ''; // 同源（vite proxy 转发 /api → bagu serve）

export class HttpError extends Error {
  status: number;
  code: number;
  detail: string;
  constructor(status: number, code: number, detail: string, msg: string) {
    super(msg);
    this.status = status;
    this.code = code;
    this.detail = detail;
  }
}

async function request<T>(path: string, init?: RequestInit): Promise<T> {
  const res = await fetch(BASE + path, {
    headers: { 'Content-Type': 'application/json', ...(init?.headers || {}) },
    ...init,
  });

  const text = await res.text();
  let body: any = null;
  if (text) {
    try { body = JSON.parse(text); } catch { /* ignore */ }
  }

  if (!res.ok) {
    const e = (body as ApiError | null)?.error;
    throw new HttpError(
      res.status,
      e?.code ?? -1,
      e?.detail ?? '',
      e?.message ?? `HTTP ${res.status}`
    );
  }
  return body as T;
}

export const api = {
  get<T>(path: string): Promise<T> {
    return request<T>(path);
  },
  post<T>(path: string, body: unknown): Promise<T> {
    return request<T>(path, { method: 'POST', body: JSON.stringify(body) });
  },
  put<T>(path: string, body: unknown): Promise<T> {
    return request<T>(path, { method: 'PUT', body: JSON.stringify(body) });
  },
};

/// 流式读取 SSE 响应
///
/// 用 fetch 而非 EventSource：
///   1. EventSource 不支持 POST 与自定义 body
///   2. 我们的 SSE 只发 `data:` 行，无 `event:`，fetch 解析更直接
///
/// onEvent 收到每条 SSE 消息（已 JSON.parse）。
/// 解析失败的行被忽略。返回 promise 在流结束时 resolve。
export async function streamSSE(
  path: string,
  init: RequestInit,
  onEvent: (e: any) => void,
  signal?: AbortSignal
): Promise<void> {
  const res = await fetch(BASE + path, {
    headers: {
      'Content-Type': 'application/json',
      Accept: 'text/event-stream',
      ...(init.headers || {}),
    },
    signal,
    ...init,
  });

  if (!res.ok) {
    const text = await res.text();
    let body: any = null;
    try { body = JSON.parse(text); } catch { /* ignore */ }
    const e = (body as ApiError | null)?.error;
    throw new HttpError(
      res.status,
      e?.code ?? -1,
      e?.detail ?? '',
      e?.message ?? `HTTP ${res.status}`
    );
  }

  const reader = res.body?.getReader();
  if (!reader) throw new Error('streaming body unavailable');
  const decoder = new TextDecoder('utf-8');
  let buffer = '';

  while (true) {
    const { done, value } = await reader.read();
    if (done) break;
    buffer += decoder.decode(value, { stream: true });

    // SSE 帧以 \n\n 分隔
    let sep: number;
    while ((sep = buffer.indexOf('\n\n')) !== -1) {
      const frame = buffer.slice(0, sep);
      buffer = buffer.slice(sep + 2);
      // 单帧可能含多行 data:；累积起来
      const dataLines = frame
        .split('\n')
        .filter((l) => l.startsWith('data:'))
        .map((l) => l.slice(5).trimStart());
      if (dataLines.length === 0) continue;
      const payload = dataLines.join('\n');
      try {
        onEvent(JSON.parse(payload));
      } catch {
        // 忽略解析失败的帧
      }
    }
  }
}
