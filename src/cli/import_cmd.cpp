#include "cli/import_cmd.h"

#include <iostream>

#include "bagu/error.h"
#include "db/database.h"
#include "db/migrations.h"
#include "service/import_service.h"
#include "util/path.h"

namespace bagu::cli {

int run_import(const std::string& path, bool force) {
    auto db_path = util::default_db_path();

    // 数据目录必须存在（bagu init 之后）
    if (!std::filesystem::exists(db_path.parent_path())) {
        std::cerr << "Error: 数据目录不存在 (E"
                  << static_cast<int>(E::kDbNotFound) << ")\n\n"
                  << "  path: " << db_path.parent_path().string() << "\n\n"
                  << "提示：请先运行 `bagu init`\n";
        return to_exit_code(static_cast<int>(E::kDbNotFound));
    }

    auto db_r = db::Database::open(db_path);
    if (db_r.is_err()) {
        std::cerr << "Error: " << db_r.error().message << "\n  "
                  << db_r.error().detail << "\n";
        return to_exit_code(db_r.error().code);
    }
    auto& db = db_r.value();

    db::register_all_migrations(db);
    auto m = db.migrate();
    if (m.is_err()) {
        std::cerr << "Error: schema 初始化失败: "
                  << m.error().message << "\n  " << m.error().detail << "\n";
        return to_exit_code(m.error().code);
    }

    service::ImportService svc(db);
    service::ImportService::Options opts;
    opts.force = force;

    std::cout << "Importing from " << path << "...\n";
    auto r = svc.import_path(path, opts);
    if (r.is_err()) {
        std::cerr << "Error: " << r.error().message
                  << " (E" << r.error().code << ")\n  "
                  << r.error().detail << "\n";
        return to_exit_code(r.error().code);
    }
    auto& summary = r.value();

    std::cout << "Found " << summary.total_files << " markdown file(s).\n\n";

    int idx = 0;
    for (const auto& f : summary.files) {
        idx++;
        std::cout << "  [" << idx << "/" << summary.total_files << "] "
                  << f.file_name;
        if (!f.error.empty()) {
            std::cout << "  → \033[31mERROR: " << f.error << "\033[0m\n";
        } else if (f.skipped) {
            std::cout << "  → " << f.topic_name << " (unchanged, skip)\n";
        } else if (f.updated) {
            std::cout << "  → " << f.topic_name << " (updated, "
                      << f.cards_added << " cards)\n";
        } else {
            std::cout << "  → " << f.topic_name << " (new, "
                      << f.cards_added << " cards)\n";
        }
    }

    std::cout << "\nDone in " << summary.elapsed_seconds << "s.\n";
    std::cout << "Succeeded: " << summary.succeeded
              << ", Skipped: " << summary.skipped
              << ", Failed: " << summary.failed
              << ", Total cards: " << summary.total_cards << "\n";

    return summary.failed > 0 ? to_exit_code(static_cast<int>(E::kParseFailed)) : 0;
}

}  // namespace bagu::cli
