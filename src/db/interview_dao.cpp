#include "db/interview_dao.h"

namespace bagu::db {

Result<int64_t> InterviewDao::create_session(const InterviewSession& s) {
    auto stmt = db_.prepare(
        "INSERT INTO interview_session "
        "(topic, started_at, ended_at, total_score, question_count, "
        " llm_provider, llm_model) "
        "VALUES (?, ?, NULL, NULL, 0, ?, ?)");
    if (!stmt) return make_err<int64_t>(E::kDbPrepareFailed, "prepare failed");

    stmt.bind(1, s.topic);
    stmt.bind(2, s.started_at);
    stmt.bind(3, s.llm_provider);
    stmt.bind(4, s.llm_model);

    if (stmt.execute() < 0) return make_err<int64_t>(E::kDbExecuteFailed, "insert failed");
    return db_.last_insert_rowid();
}

Result<void> InterviewDao::finalize_session(int64_t id, int64_t ended_at,
                                            double total_score, int question_count) {
    auto stmt = db_.prepare(
        "UPDATE interview_session SET ended_at = ?, total_score = ?, "
        "question_count = ? WHERE id = ?");
    if (!stmt) return make_err(E::kDbPrepareFailed, "prepare failed");

    stmt.bind(1, ended_at);
    stmt.bind(2, total_score);
    stmt.bind(3, question_count);
    stmt.bind(4, id);

    if (stmt.execute() < 0) return make_err(E::kDbExecuteFailed, "update failed");
    return Result<void>::ok();
}

Result<int64_t> InterviewDao::add_qa(const InterviewQA& qa) {
    auto stmt = db_.prepare(
        "INSERT INTO interview_qa "
        "(session_id, question_no, question, user_answer, ai_score, "
        " ai_feedback, duration_ms) "
        "VALUES (?, ?, ?, ?, ?, ?, ?)");
    if (!stmt) return make_err<int64_t>(E::kDbPrepareFailed, "prepare failed");

    stmt.bind(1, qa.session_id);
    stmt.bind(2, qa.question_no);
    stmt.bind(3, qa.question);
    stmt.bind(4, qa.user_answer);
    stmt.bind(5, qa.ai_score);
    stmt.bind(6, qa.ai_feedback);
    stmt.bind(7, qa.duration_ms);

    if (stmt.execute() < 0) return make_err<int64_t>(E::kDbExecuteFailed, "insert failed");
    return db_.last_insert_rowid();
}

Result<std::optional<InterviewSession>> InterviewDao::find_session(int64_t id) {
    auto stmt = db_.prepare(
        "SELECT id, topic, started_at, IFNULL(ended_at, 0), "
        "IFNULL(total_score, 0), question_count, "
        "IFNULL(llm_provider, ''), IFNULL(llm_model, '') "
        "FROM interview_session WHERE id = ?");
    if (!stmt) return make_err<std::optional<InterviewSession>>(
        E::kDbPrepareFailed, "prepare failed");
    stmt.bind(1, id);
    if (!stmt.step()) return std::optional<InterviewSession>(std::nullopt);

    InterviewSession s;
    s.id            = stmt.column_int64(0);
    s.topic         = stmt.column_text(1);
    s.started_at    = stmt.column_int64(2);
    s.ended_at      = stmt.column_int64(3);
    s.total_score   = stmt.column_double(4);
    s.question_count = stmt.column_int(5);
    s.llm_provider  = stmt.column_text(6);
    s.llm_model     = stmt.column_text(7);
    return std::optional<InterviewSession>(s);
}

Result<std::vector<InterviewQA>> InterviewDao::qa_list(int64_t session_id) {
    std::vector<InterviewQA> out;
    auto stmt = db_.prepare(
        "SELECT id, session_id, question_no, question, "
        "IFNULL(user_answer, ''), IFNULL(ai_score, 0), "
        "IFNULL(ai_feedback, ''), IFNULL(duration_ms, 0) "
        "FROM interview_qa WHERE session_id = ? "
        "ORDER BY question_no ASC");
    if (!stmt) return make_err<std::vector<InterviewQA>>(
        E::kDbPrepareFailed, "prepare failed");
    stmt.bind(1, session_id);

    while (stmt.step()) {
        InterviewQA q;
        q.id          = stmt.column_int64(0);
        q.session_id  = stmt.column_int64(1);
        q.question_no = stmt.column_int(2);
        q.question    = stmt.column_text(3);
        q.user_answer = stmt.column_text(4);
        q.ai_score    = stmt.column_int(5);
        q.ai_feedback = stmt.column_text(6);
        q.duration_ms = stmt.column_int(7);
        out.push_back(std::move(q));
    }
    return out;
}

Result<std::vector<InterviewSession>> InterviewDao::recent_sessions(int limit) {
    std::vector<InterviewSession> out;
    auto stmt = db_.prepare(
        "SELECT id, topic, started_at, IFNULL(ended_at, 0), "
        "IFNULL(total_score, 0), question_count, "
        "IFNULL(llm_provider, ''), IFNULL(llm_model, '') "
        "FROM interview_session ORDER BY started_at DESC LIMIT ?");
    if (!stmt) return make_err<std::vector<InterviewSession>>(
        E::kDbPrepareFailed, "prepare failed");
    stmt.bind(1, limit);

    while (stmt.step()) {
        InterviewSession s;
        s.id            = stmt.column_int64(0);
        s.topic         = stmt.column_text(1);
        s.started_at    = stmt.column_int64(2);
        s.ended_at      = stmt.column_int64(3);
        s.total_score   = stmt.column_double(4);
        s.question_count = stmt.column_int(5);
        s.llm_provider  = stmt.column_text(6);
        s.llm_model     = stmt.column_text(7);
        out.push_back(std::move(s));
    }
    return out;
}

}  // namespace bagu::db
