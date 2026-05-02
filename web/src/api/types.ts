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

export interface ReviewState {
  card_id: number;
  last_review: number;
  next_review: number;
  interval_days: number;
  ease_factor: number;
  repetitions: number;
  review_count: number;
  correct_count: number;
  suspended: boolean;
}

export interface DueCard {
  card_id: number;
  topic_id: number;
  topic_name: string;
  question: string;
  answer: string;
  card_type: string;
  is_new: boolean;
  review: ReviewState;
}

export interface DueResponse {
  topic: string;
  max_due: number;
  max_new: number;
  cards: DueCard[];
}

// ===== 面试 =====

export interface InterviewSession {
  id: number;
  topic: string;
  started_at: number;
  ended_at: number;
  total_score: number;
  question_count: number;
  llm_provider: string;
  llm_model: string;
}

export interface InterviewQA {
  id: number;
  session_id: number;
  question_no: number;
  question: string;
  user_answer: string;
  ai_score: number;
  ai_feedback: string;
  duration_ms: number;
}

export interface InterviewSessionDetail extends InterviewSession {
  qas: InterviewQA[];
}

export interface CreateSessionResponse {
  session_id: number;
  topic: string;
  question_count: number;
  provider: string;
  model: string;
}

export interface FinishSessionResponse {
  session_id: number;
  answered: number;
  avg_score: number;
  total_score: number;
}

// ===== 统计 =====

export interface OverallStats {
  total_topics: number;
  total_cards: number;
  total_reviews: number;
  total_correct: number;
  learned_unique_cards: number;
  overall_accuracy: number;
  streak_days: number;
  active_days_30: number;
}

export interface TopicProgress {
  topic_id: number;
  topic_name: string;
  title: string;
  total_cards: number;
  learned_cards: number;
  correct_cards: number;
  accuracy: number;
  due_today: number;
}

export interface DailyCount {
  date: string;     // YYYY-MM-DD
  count: number;
}

export interface HeatmapResponse {
  days: number;
  items: DailyCount[];
}

export interface WeakCard {
  card_id: number;
  topic_name: string;
  question: string;
  wrong_count: number;
  total_recent: number;
}

/// SSE 流事件
export type StreamEvent =
  | { type: 'chunk'; text: string }
  | { type: 'done'; question_no: number; content?: string; score?: number; feedback?: string; qa_id?: number }
  | { type: 'error'; message: string; detail?: string };
