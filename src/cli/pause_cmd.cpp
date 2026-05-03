#include "cli/pause_cmd.h"

#include <iostream>

#include "bagu/error.h"
#include "db/database.h"
#include "db/migrations.h"
#include "db/review_dao.h"
#include "db/topic_dao.h"
#include "util/path.h"

namespace bagu::cli {

int run_pause(const PauseCliOptions& opts) {
    if (opts.card_id == 0 && opts.topic.empty()) {
        std::cerr << "Error: 需要 <card_id> 或 --topic <T>\n"
                  << "用法：bagu pause <card_id>      # 暂停单卡\n"
                  << "      bagu pause --topic mysql  # 暂停整个主题\n"
                  << "      bagu unpause <card_id>    # 恢复\n";
        return to_exit_code(static_cast<int>(E::kArgRequired));
    }

    auto db_path = util::default_db_path();
    if (!std::filesystem::exists(db_path)) {
        std::cerr << "Error: 数据库不存在 (E"
                  << static_cast<int>(E::kDbNotFound) << ")\n";
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

    db::ReviewDao rdao(db);
    const char* verb = opts.unpause ? "恢复" : "暂停";

    if (opts.card_id > 0) {
        auto r = rdao.set_suspended(opts.card_id, !opts.unpause);
        if (r.is_err()) {
            std::cerr << "Error: " << r.error().message << "\n";
            return to_exit_code(r.error().code);
        }
        std::cout << "已" << verb << " card #" << opts.card_id << "\n";
        return 0;
    }

    // by topic
    auto t_r = db::TopicDao(db).find_by_name(opts.topic);
    if (t_r.is_err()) {
        std::cerr << "Error: " << t_r.error().message << "\n";
        return to_exit_code(t_r.error().code);
    }
    if (!t_r.value().has_value()) {
        std::cerr << "Error: 主题不存在: " << opts.topic << "\n";
        return to_exit_code(static_cast<int>(E::kTopicNotFound));
    }
    auto r = rdao.set_suspended_by_topic(t_r.value()->id, !opts.unpause);
    if (r.is_err()) {
        std::cerr << "Error: " << r.error().message << "\n";
        return to_exit_code(r.error().code);
    }
    std::cout << "已" << verb << " topic '" << opts.topic
              << "' 下 " << r.value() << " 张卡\n";
    return 0;
}

}  // namespace bagu::cli
