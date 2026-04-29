#include "llm/prompts.h"

#include <sstream>

namespace bagu::llm::prompts {

std::string system_prompt(const std::string& topic,
                         const std::vector<std::string>& sample_cards) {
    std::ostringstream ss;
    ss << "你是一名资深的后端工程师面试官，擅长 " << topic << " 相关知识。\n"
       << "\n"
       << "你的任务是用 " << topic << " 主题的题目进行模拟面试，并对用户答案做出客观评分。\n"
       << "\n"
       << "## 规则\n"
       << "1. 一次只问一个问题，问题简洁清晰\n"
       << "2. 难度从中等开始，根据用户表现自适应\n"
       << "3. 评分用 0-10 分制：\n"
       << "   - 0-3 分：完全错或没说到关键点\n"
       << "   - 4-6 分：答到部分要点，遗漏明显\n"
       << "   - 7-8 分：核心要点齐全，细节略缺\n"
       << "   - 9-10 分：全面深入，涉及边界情况\n"
       << "4. 评分后简要给出：✓ 答对的点 / ✗ 缺漏 / 💡 建议\n"
       << "5. 不要主动说\"下一题\"等过渡语，由系统控制流程\n"
       << "\n";

    if (!sample_cards.empty()) {
        ss << "## 参考题库（仅供参考，可发挥）\n";
        size_t take = std::min(sample_cards.size(), size_t{8});
        for (size_t i = 0; i < take; ++i) {
            ss << "- " << sample_cards[i] << "\n";
        }
        ss << "\n";
    }

    ss << "请用中文与用户交流。";
    return ss.str();
}

std::string next_question_prompt(const std::string& history,
                                const std::string& difficulty_hint) {
    std::ostringstream ss;
    ss << "请根据以下历史出下一道题。\n";
    if (!history.empty()) {
        ss << "\n## 历史\n" << history << "\n";
    }
    if (!difficulty_hint.empty()) {
        ss << "\n## 难度提示\n" << difficulty_hint << "\n";
    }
    ss << "\n输出格式：直接给出题目（不带 \"题目：\"、\"Q1:\" 等前缀）。";
    return ss.str();
}

std::string grading_prompt(const std::string& question,
                          const std::string& user_answer) {
    std::ostringstream ss;
    ss << "## 当前问题\n" << question << "\n\n"
       << "## 用户答案\n" << (user_answer.empty() ? "(用户未作答)" : user_answer) << "\n\n"
       << "请按以下格式严格输出（必须 4 段，按顺序）：\n"
       << "评分：N/10\n"
       << "✓ <答对的点（一行；如无写 \"无\"）>\n"
       << "✗ <遗漏或错误（一行；如无写 \"无\"）>\n"
       << "💡 <一句建议>\n";
    return ss.str();
}

}  // namespace bagu::llm::prompts
