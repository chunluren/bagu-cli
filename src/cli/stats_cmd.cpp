#include "cli/stats_cmd.h"

#include <unistd.h>

#include <ctime>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>

#include "bagu/error.h"
#include "db/database.h"
#include "service/stats_service.h"
#include "util/path.h"

namespace bagu::cli {

namespace {

bool is_tty() { return ::isatty(STDOUT_FILENO); }

struct Colors {
    bool enabled;
    const char* bold()    const { return enabled ? "\033[1m" : ""; }
    const char* dim()     const { return enabled ? "\033[2m" : ""; }
    const char* yellow()  const { return enabled ? "\033[33m" : ""; }
    const char* cyan()    const { return enabled ? "\033[36m" : ""; }
    const char* green()   const { return enabled ? "\033[32m" : ""; }
    const char* red()     const { return enabled ? "\033[31m" : ""; }
    const char* reset()   const { return enabled ? "\033[0m" : ""; }
};

/// 把日期字符串 "YYYY-MM-DD" 解析为 tm
std::tm parse_date(const std::string& s) {
    std::tm t{};
    sscanf(s.c_str(), "%d-%d-%d", &t.tm_year, &t.tm_mon, &t.tm_mday);
    t.tm_year -= 1900;
    t.tm_mon -= 1;
    std::mktime(&t);  // 自动填充 wday
    return t;
}

/// 渲染热力图：按 weekday 分行（周一到周日）
void render_heatmap(const std::vector<service::DailyCount>& daily,
                   const Colors& C) {
    // 块字符梯度
    static const char* kBlocks[] = {
        " ",   // 0
        "·",   // 1
        "▁",   // 2-4
        "▃",   // 5-9
        "▅",   // 10-19
        "▇",   // 20-39
        "█",   // 40+
    };
    auto pick_block = [&](int n) -> int {
        if (n == 0) return 0;
        if (n == 1) return 1;
        if (n <= 4) return 2;
        if (n <= 9) return 3;
        if (n <= 19) return 4;
        if (n <= 39) return 5;
        return 6;
    };
    auto pick_color = [&](int n) -> const char* {
        if (n == 0) return "";
        if (n <= 4) return C.dim();
        if (n <= 19) return C.green();
        return C.cyan();
    };

    // 把日期按周组织：每周 7 天，跨多列
    // weekday 的 0=周日, 1=周一, ..., 6=周六；我们按周一到周日排
    // 第一列对应最早的周，最后一列对应当前周
    std::map<std::pair<int, int>, int> grid; // (week_index, weekday) -> count

    if (daily.empty()) return;

    auto first_date = parse_date(daily.front().date);
    int first_wday = (first_date.tm_wday + 6) % 7; // Mon=0..Sun=6

    int max_week = 0;
    for (size_t i = 0; i < daily.size(); ++i) {
        int day_offset = static_cast<int>(i) + first_wday;
        int week = day_offset / 7;
        int wday = day_offset % 7;
        grid[{week, wday}] = daily[i].count;
        max_week = std::max(max_week, week);
    }

    // 月份标签（每月第一次出现的位置）
    // 简化：跳过
    static const char* kWeekDays[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};

    std::cout << "\n  " << C.dim() << "（" << daily.size() << " days）"
              << C.reset() << "\n";

    for (int wd = 0; wd < 7; ++wd) {
        std::cout << "  " << C.dim() << kWeekDays[wd] << C.reset() << " ";
        for (int w = 0; w <= max_week; ++w) {
            auto it = grid.find({w, wd});
            int n = it == grid.end() ? 0 : it->second;
            const char* col = pick_color(n);
            std::cout << col << kBlocks[pick_block(n)] << C.reset();
        }
        std::cout << "\n";
    }
    std::cout << "\n  " << C.dim() << "Less " << C.reset()
              << kBlocks[1] << kBlocks[2] << " "
              << C.green() << kBlocks[3] << kBlocks[4] << C.reset() << " "
              << C.cyan() << kBlocks[5] << kBlocks[6] << C.reset()
              << " " << C.dim() << "More" << C.reset() << "\n";
}

void render_overall(const service::OverallStats& s, const Colors& C) {
    std::cout << "\n" << C.bold() << "========== 总览 =========="
              << C.reset() << "\n";
    std::cout << "  连续打卡:   " << C.yellow() << s.streak_days
              << " 天" << C.reset() << "\n";
    std::cout << "  最近 30 天活跃:  " << s.active_days_30 << " 天\n";
    std::cout << "  累计复习:   " << s.total_reviews << " 次\n";
    std::cout << "  整体正确率: " << std::fixed << std::setprecision(1)
              << (s.overall_accuracy * 100) << "%\n";
    std::cout << "  已学卡片:   " << s.learned_unique_cards << " / "
              << s.total_cards << "\n";
    std::cout << "  主题数:     " << s.total_topics << "\n\n";
}

void render_per_topic(const std::vector<service::TopicProgress>& list,
                     const Colors& C) {
    if (list.empty()) {
        std::cout << "  (尚未导入任何主题)\n\n";
        return;
    }
    std::cout << C.bold() << "========== 各主题进度 =========="
              << C.reset() << "\n";
    std::cout << "  " << std::left
              << std::setw(18) << "TOPIC"
              << std::right
              << std::setw(10) << "LEARNED"
              << std::setw(10) << "ACCURACY"
              << std::setw(8) << "DUE"
              << "\n";
    std::cout << "  " << std::string(46, '-') << "\n";
    for (const auto& t : list) {
        int pct = t.total_cards > 0
            ? t.learned_cards * 100 / t.total_cards : 0;
        std::cout << "  " << std::left
                  << std::setw(18) << t.topic_name
                  << std::right
                  << std::setw(8) << t.learned_cards << "/" << std::setw(2) << t.total_cards
                  << std::setw(7) << std::fixed << std::setprecision(0)
                  << (t.accuracy * 100) << "%   ";
        if (t.due_today > 0) {
            std::cout << C.yellow() << std::setw(5) << t.due_today << C.reset();
        } else {
            std::cout << C.dim() << std::setw(5) << "-" << C.reset();
        }
        std::cout << "  " << C.dim() << "(" << pct << "%)" << C.reset() << "\n";
    }
    std::cout << "\n";
}

void render_weak_cards(const std::vector<service::WeakCard>& list,
                      const Colors& C) {
    if (list.empty()) return;
    std::cout << C.bold() << "========== 最薄弱（最近答错最多）=========="
              << C.reset() << "\n";
    int idx = 0;
    for (const auto& w : list) {
        idx++;
        std::cout << "  " << idx << ". "
                  << C.cyan() << "[" << w.topic_name << "]" << C.reset()
                  << " " << w.question << "\n"
                  << "      " << C.red() << "答错 " << w.wrong_count
                  << "/" << w.total_recent << " 次" << C.reset() << "\n";
    }
    std::cout << "\n";
}

}  // namespace

int run_stats(const StatsOptions& opts) {
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

    Colors C{is_tty()};
    service::StatsService svc(db);

    // 总览
    auto overall = svc.overall();
    if (overall.is_err()) {
        std::cerr << "Error: " << overall.error().message << "\n";
        return to_exit_code(overall.error().code);
    }
    render_overall(overall.value(), C);

    // 主题进度
    auto pt = svc.per_topic();
    if (pt.is_ok()) render_per_topic(pt.value(), C);

    // 薄弱
    auto weak = svc.weak_cards(50, 5);
    if (weak.is_ok()) render_weak_cards(weak.value(), C);

    // 热力图
    if (opts.heatmap) {
        auto daily = svc.daily_counts(opts.days);
        if (daily.is_ok()) {
            std::cout << C.bold() << "========== 热力图 (最近 "
                      << opts.days << " 天)" << C.reset() << "\n";
            render_heatmap(daily.value(), C);
        }
    }
    return 0;
}

}  // namespace bagu::cli
