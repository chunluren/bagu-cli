#pragma once

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "bagu/result.h"
#include "db/database.h"
#include "db/interview_dao.h"
#include "llm/llm_client.h"

namespace bagu::service {

struct InterviewOptions {
    std::string topic;        // 主题名（必需）
    int question_count = 5;   // 总题数
};

/// 单次面试结果
struct InterviewResult {
    int64_t session_id = 0;
    int total_questions = 0;
    int answered = 0;
    double avg_score = 0.0;
};

/// 评分解析结果
struct GradeResult {
    int score = 0;            // 0-10
    std::string feedback;     // 完整反馈文本
};

class InterviewService {
public:
    InterviewService(db::Database& db, std::unique_ptr<llm::LLMClient> llm)
        : db_(db), llm_(std::move(llm)) {}

    /// 创建会话
    Result<int64_t> start_session(const InterviewOptions& opts,
                                  const std::string& llm_model);

    /// 取下一题（流式）
    /// @param question_no 当前题号（1-based）
    /// @param previous_summary 上一题摘要（"Q: ... A: ... 评分 7"）
    /// @param on_chunk 流式片段回调
    Result<std::string> next_question(int question_no,
                                     const std::string& previous_summary,
                                     std::function<void(const std::string&)> on_chunk);

    /// 评分用户答案（流式）
    Result<GradeResult> grade_answer(const std::string& question,
                                    const std::string& user_answer,
                                    std::function<void(const std::string&)> on_chunk);

    /// 保存一条问答
    Result<void> save_qa(int64_t session_id, int question_no,
                        const std::string& question, const std::string& user_answer,
                        const GradeResult& grade, int duration_ms);

    /// 结束会话（写 ended_at + 总分）
    Result<void> end_session(int64_t session_id, double avg_score, int question_count);

    /// 获取主题相关样本卡片（供 system prompt 用）
    Result<std::vector<std::string>> sample_topic_cards(const std::string& topic, int limit);

    /// 解析评分文本：抓 "评分：N/10"
    static int parse_score(const std::string& text);

    llm::LLMClient* llm() { return llm_.get(); }

private:
    db::Database& db_;
    std::unique_ptr<llm::LLMClient> llm_;

    // 缓存系统 prompt（避免每题重新拼）
    std::string system_prompt_;
    std::string current_topic_;
};

}  // namespace bagu::service
