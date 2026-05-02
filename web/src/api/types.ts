// 与后端 src/http/json_io.h / src/db/models.h 对齐

export interface ApiError {
  error: { code: number; message: string; detail: string };
}

export interface Topic {
  id: number;
  name: string;
  title: string;
  file_path: string;
  file_hash: string;
  imported_at: number;
  updated_at: number;
  cards?: number;
  chapters?: number;
}

export interface Chapter {
  id: number;
  topic_id: number;
  name: string;
  chapter_no: number;
  parent_id: number | null;
}

/// /api/topics/:name 返回（chapters 字段是数组而非数量）
export interface TopicDetail extends Omit<Topic, 'chapters'> {
  chapters: Chapter[];
}

export interface Card {
  id: number;
  topic_id: number;
  chapter_id: number | null;
  question: string;
  answer: string;
  difficulty: number;
  tags: string;
  source_line: number;
  card_type: 'qa' | 'section' | 'code' | string;
  created_at: number;
  updated_at: number;
}

export interface SearchResult {
  query: string;
  took_ms: number;
  items: Card[];
}

export interface HealthResponse {
  ok: boolean;
  version: string;
  schema_version: number;
}
