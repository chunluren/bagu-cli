#include "cli/export_cmd.h"

#include <fstream>
#include <iostream>

#include "bagu/error.h"
#include "db/database.h"
#include "db/migrations.h"
#include "service/export_service.h"
#include "util/path.h"

namespace bagu::cli {

int run_export(const ExportCliOptions& opts) {
    if (opts.format != "anki" && opts.format != "csv") {
        std::cerr << "Error: 不支持的格式: " << opts.format
                  << "（支持: anki / csv）\n";
        return to_exit_code(static_cast<int>(E::kArgInvalidValue));
    }

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

    service::ExportService svc(db);

    auto write = [&](std::ostream& out) {
        if (opts.format == "csv") {
            service::CsvExportOptions copts;
            copts.topic = opts.topic;
            copts.include_section_cards = opts.include_section_cards;
            return svc.export_csv(copts, out);
        }
        service::AnkiExportOptions aopts;
        aopts.topic = opts.topic;
        aopts.include_section_cards = opts.include_section_cards;
        return svc.export_anki(aopts, out);
    };

    service::AnkiExportSummary sum;
    if (opts.output.empty()) {
        auto r = write(std::cout);
        if (r.is_err()) {
            std::cerr << "Error: " << r.error().message << "\n";
            return to_exit_code(r.error().code);
        }
        sum = r.value();
    } else {
        std::ofstream f(opts.output, std::ios::binary | std::ios::trunc);
        if (!f) {
            std::cerr << "Error: 无法写入 " << opts.output << "\n";
            return to_exit_code(static_cast<int>(E::kFileNotFound));
        }
        auto r = write(f);
        if (r.is_err()) {
            std::cerr << "Error: " << r.error().message << "\n";
            return to_exit_code(r.error().code);
        }
        sum = r.value();
    }

    // 总结写到 stderr，不污染 stdout
    std::cerr << "[export " << opts.format << "] total=" << sum.total_cards
              << " written=" << sum.written
              << " skipped=" << sum.skipped;
    if (!opts.output.empty()) std::cerr << " → " << opts.output;
    std::cerr << "\n";
    return 0;
}

}  // namespace bagu::cli
