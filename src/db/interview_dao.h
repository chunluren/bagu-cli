#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "bagu/result.h"
#include "db/database.h"

namespace bagu::db {

struct InterviewSession {
    int64_t id = 0;
    std::string topic;
    int64_t started_at = 0;
    int64_t ended_at = 0;
    double total_score = 0.0;
    int question_count = 0;
    std::string llm_provider;
    std::string llm_model;
};

struct InterviewQA {
    int64_t id = 0;
    int64_t session_id = 0;
    int question_no = 0;
    std::string question;
    std::string user_answer;
    int ai_score = 0;
    std::string ai_feedback;
    int duration_ms = 0;
};

class InterviewDao {
public:
    explicit InterviewDao(Database& db) : db_(db) {}

    Result<int64_t> create_session(const InterviewSession& s);
    Result<void> finalize_session(int64_t id, int64_t ended_at,
                                  double total_score, int question_count);

    Result<int64_t> add_qa(const InterviewQA& qa);

    Result<std::optional<InterviewSession>> find_session(int64_t id);
    Result<std::vector<InterviewQA>> qa_list(int64_t session_id);
    Result<std::vector<InterviewSession>> recent_sessions(int limit);

private:
    Database& db_;
};

}  // namespace bagu::db
