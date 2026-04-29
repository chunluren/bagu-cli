#pragma once

#include <string>
#include <vector>

namespace bagu::llm::prompts {

/// 拼装系统 prompt
/// @param topic 主题（如 "MySQL"）
/// @param sample_cards 主题相关示例卡片（用作 AI 出题参考，节选）
std::string system_prompt(const std::string& topic,
                          const std::vector<std::string>& sample_cards);

/// 出下一题的 prompt
/// @param history 已有问答历史摘要（"Q1: ... | 答得 7/10\nQ2: ..."）
/// @param difficulty_hint 难度调整提示
std::string next_question_prompt(const std::string& history,
                                 const std::string& difficulty_hint);

/// 评分 prompt
std::string grading_prompt(const std::string& question,
                          const std::string& user_answer);

}  // namespace bagu::llm::prompts
