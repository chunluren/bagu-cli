#pragma once

#include <vector>

#include "db/review_dao.h"
#include "service/review_service.h"

namespace bagu::tui {

struct ReviewSessionStats {
    int total = 0;          // 计划复习数
    int reviewed = 0;       // 实际完成数
    int correct = 0;        // 评分 >= 3
    int quit_early = 0;     // 中途退出未复习的剩余数
};

/// 启动复习 TUI
/// @return 用户交互完后的统计
ReviewSessionStats run_review_tui(service::ReviewService& svc,
                                  std::vector<db::DueCard> cards);

}  // namespace bagu::tui
