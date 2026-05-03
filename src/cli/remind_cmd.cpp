#include "cli/remind_cmd.h"

#include <iostream>
#include <sstream>

#include "bagu/error.h"
#include "db/database.h"
#include "db/migrations.h"
#include "service/review_service.h"
#include "util/notify.h"
#include "util/path.h"

namespace bagu::cli {

int run_remind(const RemindCliOptions& opts) {
    auto db_path = util::default_db_path();
    if (!std::filesystem::exists(db_path)) {
        if (!opts.quiet) {
            std::cerr << "Error: 数据库不存在 (E"
                      << static_cast<int>(E::kDbNotFound) << ")\n";
        }
        return to_exit_code(static_cast<int>(E::kDbNotFound));
    }
    auto db_r = db::Database::open(db_path);
    if (db_r.is_err()) {
        if (!opts.quiet) std::cerr << "Error: " << db_r.error().message << "\n";
        return to_exit_code(db_r.error().code);
    }
    auto& db = db_r.value();
    db::register_all_migrations(db);
    auto m = db.migrate();
    if (m.is_err()) {
        if (!opts.quiet) std::cerr << "Error: " << m.error().message << "\n";
        return to_exit_code(m.error().code);
    }

    auto sum_r = service::ReviewService(db).due_summary();
    if (sum_r.is_err()) {
        if (!opts.quiet) std::cerr << "Error: " << sum_r.error().message << "\n";
        return to_exit_code(sum_r.error().code);
    }
    auto& sum = sum_r.value();

    // 按 --topic 过滤（如果指定了）
    int total_due = sum.total_due;
    int total_new = sum.total_new;
    std::vector<service::DueByTopic> matched;
    if (!opts.topic.empty()) {
        total_due = 0; total_new = 0;
        for (const auto& t : sum.per_topic) {
            if (t.topic_name == opts.topic) {
                matched.push_back(t);
                total_due += t.due;
                total_new += t.new_cards;
            }
        }
    } else {
        matched = sum.per_topic;
    }

    int total_pending = total_due + total_new;
    if (total_pending < opts.threshold) {
        if (!opts.quiet) {
            std::cout << "今日待复习 " << total_pending
                      << " < threshold " << opts.threshold << "，不发通知。\n";
        }
        return 0;
    }

    // 构建消息
    std::ostringstream title_ss;
    title_ss << "bagu：今日有 " << total_due << " 张卡到期";
    if (total_new > 0) title_ss << " + " << total_new << " 张新卡";

    std::ostringstream body_ss;
    int shown = 0;
    for (const auto& t : matched) {
        if (t.due == 0 && t.new_cards == 0) continue;
        if (shown > 0) body_ss << "\n";
        body_ss << t.topic_name << "：到期 " << t.due
                << " 新卡 " << t.new_cards;
        if (++shown >= 4) {
            int rest = static_cast<int>(matched.size()) - shown;
            if (rest > 0) body_ss << "\n... 还有 " << rest << " 个主题";
            break;
        }
    }

    std::string title = title_ss.str();
    std::string body  = body_ss.str();

    if (opts.dry_run) {
        std::cout << "[dry-run]\n  title: " << title << "\n  body: " << body << "\n";
        return 0;
    }

    if (!util::desktop_notify_available()) {
        if (!opts.quiet) {
            std::cerr << "warn: 桌面通知不可用（headless / 缺命令）\n"
                      << title << "\n" << body << "\n";
        }
        return to_exit_code(static_cast<int>(E::kFileNotFound));
    }

    auto r = util::notify_desktop(title, body);
    if (r.is_err()) {
        if (!opts.quiet) {
            std::cerr << "Error: 发送通知失败: " << r.error().message << "\n";
        }
        return to_exit_code(r.error().code);
    }
    if (!opts.quiet) {
        std::cout << "已发通知：" << title << "\n";
    }
    return 0;
}

}  // namespace bagu::cli
