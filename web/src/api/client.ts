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
