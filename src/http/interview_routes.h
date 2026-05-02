#pragma once

#include <httplib.h>

#include "db/database.h"

namespace bagu::http {

/// 在已有 server 上注册 interview 相关路由（仅在 CURL 编译时可用）
///
/// 路由：
///   POST /api/interview/sessions               创建会话（返回 session_id + system_prompt 摘要）
///   GET  /api/interview/sessions               列最近会话
///   GET  /api/interview/sessions/:id           获取会话详情 + qa list
///   GET  /api/interview/sessions/:id/question  SSE 流式出下一题
///   POST /api/interview/sessions/:id/answer    SSE 流式评分（保存 qa）
///   POST /api/interview/sessions/:id/finish    结束会话（写 ended_at + avg_score）
///
/// SSE 事件：
///   data: {"type":"chunk","text":"..."}\n\n   增量片段
///   data: {"type":"done","content":"...full","score":7,"qa_id":42}\n\n   完成（评分接口含 score）
///   data: {"type":"error","message":"..."}\n\n   错误
void register_interview_routes(httplib::Server& svr, db::Database& db);

}  // namespace bagu::http
