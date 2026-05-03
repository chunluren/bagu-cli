#include "cli/due_cmd.h"

#include <unistd.h>

#include <iostream>

#include "bagu/error.h"
#include "db/database.h"
#include "db/migrations.h"
#include "service/review_service.h"
#include "util/path.h"

namespace bagu::cli {

namespace {

struct Colors {
    bool enabled;
    const char* bold()    const { return enabled ? "\033[1m" : ""; }
    const char* dim()     const { return enabled ? "\033[2m" : ""; }
    const char* cyan()    const { return enabled ? "\033[36m" : ""; }
    const char* yellow()  const { return enabled ? "\033[33m" : ""; }
    const char* green()   const { return enabled ? "\033[32m" : ""; }
    const char* reset()   const { return enabled ? "\033[0m" : ""; }
};

}  // namespace

int run_due(const DueCliOptions& opts) {
    auto db_path = util::default_db_path();
    if (!std::filesystem::exists(db_path)) {
        std::cerr << "Error: 数据库不存在 (E"
                  << static_cast<int>(E::kDbNotFound) << ")\n"
                  << "提示：先 `bagu init && bagu import <path>`\n";
        return to_exit_code(static_cast<int>(E::kDbNotFound));
    }
    auto db_r = db::Database::open(db_path);
    if (db_r.is_err()) {
        std::cerr << "Error: " << db_r.error().message << "\n";
        return to_exit_code(db_r.error().code);
    }
    auto& db = db_r.value();
    db::register_all_migrations(db);
    auto m = db.migrate();
    if (m.is_err()) {
        std::cerr << "Error: " << m.error().message << "\n";
        return to_exit_code(m.error().code);
    }

    service::ReviewService svc(db);
    auto r = svc.due_summary();
    if (r.is_err()) {
        std::cerr << "Error: " << r.error().message << "\n";
        return to_exit_code(r.error().code);
    }
    auto& sum = r.value();

    Colors C{::isatty(1) != 0};

    if (sum.total_due == 0 && sum.total_new == 0) {
        std::cout << C.green() << "🎉 今日没有到期卡片，全部主题都已掌握或没新卡。"
                  << C.reset() << "\n";
        return 0;
    }

    std::cout << C.bold() << "今日到期" << C.reset() << "："
              << C.cyan() << sum.total_due << C.reset() << " 张待复习";
    if (sum.total_new > 0) {
        std::cout << "  +  " << C.yellow() << sum.total_new << C.reset() << " 张新卡";
    }
    std::cout << "\n\n";

    bool has_match = false;
    for (const auto& t : sum.per_topic) {
        if (!opts.topic.empty() && t.topic_name != opts.topic) continue;
        if (t.due == 0 && t.new_cards == 0) continue;
        has_match = true;
        std::cout << "  " << C.cyan() << t.topic_name << C.reset()
                  << C.dim() << " (" << t.topic_title << ")" << C.reset() << "\n"
                  << "    到期：" << C.bold() << t.due << C.reset()
                  << "    新卡：" << C.bold() << t.new_cards << C.reset() << "\n";
    }
    if (!opts.topic.empty() && !has_match) {
        std::cout << C.dim() << "（主题 " << opts.topic
                  << " 没有到期或新卡）" << C.reset() << "\n";
    }

    std::cout << "\n" << C.dim() << "下一步：" << C.reset();
    if (opts.topic.empty()) {
        std::cout << "bagu review";
    } else {
        std::cout << "bagu review --topic " << opts.topic;
    }
    std::cout << "\n";
    return 0;
}

}  // namespace bagu::cli
