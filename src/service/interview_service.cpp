#include "service/interview_service.h"

#include <ctime>
#include <regex>

#include "db/topic_dao.h"
#include "db/card_dao.h"
#include "llm/prompts.h"

namespace bagu::service {

Result<int64_t> InterviewService::start_session(const InterviewOptions& opts,
                                                const std::string& llm_model) {
    current_topic_ = opts.topic;

    // 加载主题样本卡片
    auto samples_r = sample_topic_cards(opts.topic, 8);
    std::vector<std::string> samples = samples_r.is_ok() ? samples_r.value()
                                                          : std::vector<std::string>{};
    system_prompt_ = llm::prompts::system_prompt(opts.topic, samples);

    // 写 db
    db::InterviewDao dao(db_);
    db::InterviewSession s;
    s.topic = opts.topic;
    s.started_at = static_cast<int64_t>(std::time(nullptr));
    s.llm_provider = llm_->provider_name();
    s.llm_model = llm_model;
    return dao.create_session(s);
}

Result<std::string> InterviewService::next_question(
    int question_no,
    const std::string& previous_summary,
    std::function<void(const std::string&)> on_chunk) {

    std::string difficulty_hint;
    if (question_no == 1) {
        difficulty_hint = "首题：中等难度";
    } else {
        difficulty_hint = "根据上一题表现自适应调整";
    }

    llm::ChatRequest req;
    req.messages.push_back({"system", system_prompt_});
    req.messages.push_back({"user",
        llm::prompts::next_question_prompt(previous_summary, difficulty_hint)});
    req.temperature = 0.8;   // 出题需要多样
    req.max_tokens = 256;
    req.stream = static_cast<bool>(on_chunk);

    auto r = on_chunk
        ? llm_->chat_stream(req, on_chunk)
        : llm_->chat(req);
    if (r.is_err()) return Result<std::string>(r.error());
    return r.value().content;
}

Result<GradeResult> InterviewService::grade_answer(
    const std::string& question,
    const std::string& user_answer,
    std::function<void(const std::string&)> on_chunk) {

    llm::ChatRequest req;
    req.messages.push_back({"system", system_prompt_});
    req.messages.push_back({"user",
        llm::prompts::grading_prompt(question, user_answer)});
    req.temperature = 0.3;   // 评分需要稳定
    req.max_tokens = 512;
    req.stream = static_cast<bool>(on_chunk);

    auto r = on_chunk
        ? llm_->chat_stream(req, on_chunk)
        : llm_->chat(req);
    if (r.is_err()) return Result<GradeResult>(r.error());

    GradeResult g;
    g.feedback = r.value().content;
    g.score = parse_score(g.feedback);
    return g;
}

Result<void> InterviewService::save_qa(int64_t session_id, int question_no,
                                       const std::string& question,
                                       const std::string& user_answer,
                                       const GradeResult& grade,
                                       int duration_ms) {
    db::InterviewDao dao(db_);
    db::InterviewQA qa;
    qa.session_id = session_id;
    qa.question_no = question_no;
    qa.question = question;
    qa.user_answer = user_answer;
    qa.ai_score = grade.score;
    qa.ai_feedback = grade.feedback;
    qa.duration_ms = duration_ms;
    auto r = dao.add_qa(qa);
    if (r.is_err()) return Result<void>(r.error());
    return Result<void>::ok();
}

Result<void> InterviewService::end_session(int64_t session_id,
                                           double avg_score,
                                           int question_count) {
    db::InterviewDao dao(db_);
    return dao.finalize_session(session_id,
        static_cast<int64_t>(std::time(nullptr)), avg_score, question_count);
}

Result<std::vector<std::string>>
InterviewService::sample_topic_cards(const std::string& topic, int limit) {
    db::TopicDao tdao(db_);
    auto t_r = tdao.find_by_name(topic);
    if (t_r.is_err()) return Result<std::vector<std::string>>(t_r.error());
    if (!t_r.value().has_value()) {
        return std::vector<std::string>{};
    }

    db::CardDao cdao(db_);
    auto cards_r = cdao.find_by_topic(t_r.value()->id);
    if (cards_r.is_err()) return Result<std::vector<std::string>>(cards_r.error());

    std::vector<std::string> out;
    for (const auto& c : cards_r.value()) {
        if (static_cast<int>(out.size()) >= limit) break;
        if (c.card_type == "qa") {
            out.push_back(c.question);
        }
    }
    return out;
}

int InterviewService::parse_score(const std::string& text) {
    // 匹配 "评分：N/10" 或 "评分: N/10" 或 "评分 N/10"
    static const std::regex re(R"(评分[：:\s]*([0-9]+)\s*/\s*10)");
    std::smatch m;
    if (std::regex_search(text, m, re)) {
        try {
            int score = std::stoi(m[1].str());
            if (score < 0) score = 0;
            if (score > 10) score = 10;
            return score;
        } catch (...) { return 0; }
    }
    // 备选格式 "Score: N"
    static const std::regex re2(R"((?:[Ss]core|分数)[：:\s]*([0-9]+))");
    if (std::regex_search(text, m, re2)) {
        try { return std::min(10, std::max(0, std::stoi(m[1].str()))); }
        catch (...) {}
    }
    return 0;
}

}  // namespace bagu::service
