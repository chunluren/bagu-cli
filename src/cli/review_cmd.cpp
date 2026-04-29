#include "cli/review_cmd.h"

#include <iostream>

#include "bagu/error.h"
#include "db/database.h"
#include "db/topic_dao.h"
#include "service/review_service.h"
#include "tui/review_screen.h"
#include "util/path.h"

namespace bagu::cli {

int run_review(const ReviewOptions& opts) {
    auto db_path = util::default_db_path();
    if (!std::filesystem::exists(db_path)) {
        std::cerr << "Error: 数据库不存在 (E"
                  << static_cast<int>(E::kDbNotFound) << ")\n\n"
                  << "提示：请先 `bagu init` 并 `bagu import <path>`\n";
        return to_exit_code(static_cast<int>(E::kDbNotFound));
    }

    auto db_r = db::Database::open(db_path);
    if (db_r.is_err()) {
        std::cerr << "Error: " << db_r.error().message << "\n";
        return to_exit_code(db_r.error().code);
    }
    auto& db = db_r.value();

    // topic 解析
    int64_t topic_id = 0;
    if (!opts.topic.empty()) {
        auto t_r = db::TopicDao(db).find_by_name(opts.topic);
        if (t_r.is_err()) {
            std::cerr << "Error: " << t_r.error().message << "\n";
            return to_exit_code(t_r.error().code);
        }
        if (!t_r.value().has_value()) {
            std::cerr << "Error: 主题不存在: " << opts.topic << "\n";
            return to_exit_code(static_cast<int>(E::kTopicNotFound));
        }
        topic_id = t_r.value()->id;
    }

    // 取卡片
    service::ReviewService svc(db);
    service::ReviewPlan plan;
    plan.topic_id = topic_id;

    int n = opts.num > 0 ? opts.num : 20;
    if (opts.all) n = 9999;

    if (opts.new_only) {
        plan.max_due = 0;
        plan.max_new = n;
    } else {
        plan.max_due = n;
        plan.max_new = std::min(5, n);   // 每天默认最多 5 张新卡
    }

    auto cards_r = svc.get_due_cards(plan);
    if (cards_r.is_err()) {
        std::cerr << "Error: " << cards_r.error().message << "\n";
        return to_exit_code(cards_r.error().code);
    }
    auto cards = std::move(cards_r.value());

    if (cards.empty()) {
        std::cout << "🎉 今天没有要复习的卡片了！\n\n";
        if (opts.topic.empty()) {
            std::cout << "提示：用 `bagu review --new-only` 学新卡，或者 `bagu list` 看进度\n";
        } else {
            std::cout << "提示：用 `bagu review --new-only --topic " << opts.topic
                      << "` 学新卡\n";
        }
        return 0;
    }

    std::cout << "Loading " << cards.size() << " card(s) for review...\n\n";

    auto stats = tui::run_review_tui(svc, std::move(cards));

    // 总结
    std::cout << "\n========== 复习总结 ==========\n";
    std::cout << "本轮复习: " << stats.reviewed << " / " << stats.total << "\n";
    if (stats.reviewed > 0) {
        int pct = stats.correct * 100 / stats.reviewed;
        std::cout << "正确率:   " << stats.correct << " / " << stats.reviewed
                  << " (" << pct << "%)\n";
    }
    if (stats.quit_early > 0) {
        std::cout << "未完成:   " << stats.quit_early << "\n";
    }
    std::cout << "\n";
    return 0;
}

}  // namespace bagu::cli
