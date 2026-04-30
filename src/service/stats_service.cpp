#include "service/stats_service.h"

#include <ctime>
#include <unordered_map>

namespace bagu::service {

namespace {

/// 取本地时区今日 0 点的 unix timestamp
int64_t local_midnight(int64_t ts) {
    std::time_t t = static_cast<std::time_t>(ts);
    std::tm lt{};
#if defined(_WIN32)
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    lt.tm_hour = 0;
    lt.tm_min = 0;
    lt.tm_sec = 0;
    lt.tm_isdst = -1;  // 让 mktime 自动判断 DST，避免跨夏令时切换日错位
    return static_cast<int64_t>(std::mktime(&lt));
}

std::string fmt_date(int64_t ts) {
    std::time_t t = static_cast<std::time_t>(ts);
    std::tm lt{};
#if defined(_WIN32)
    localtime_s(&lt, &t);
#else
    localtime_r(&t, &lt);
#endif
    char buf[16];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d", &lt);
    return buf;
}

}  // namespace

Result<OverallStats> StatsService::overall() {
    OverallStats s;

    {
        auto stmt = db_.prepare("SELECT COUNT(*) FROM topic");
        if (stmt && stmt.step()) s.total_topics = stmt.column_int(0);
    }
    {
        auto stmt = db_.prepare("SELECT COUNT(*) FROM card");
        if (stmt && stmt.step()) s.total_cards = stmt.column_int(0);
    }
    {
        auto stmt = db_.prepare(
            "SELECT COUNT(*), SUM(CASE WHEN score >= 3 THEN 1 ELSE 0 END) "
            "FROM review_history");
        if (stmt && stmt.step()) {
            s.total_reviews = stmt.column_int(0);
            s.total_correct = stmt.column_int(1);
        }
    }
    if (s.total_reviews > 0) {
        s.overall_accuracy =
            static_cast<double>(s.total_correct) / s.total_reviews;
    }
    {
        auto stmt = db_.prepare("SELECT COUNT(DISTINCT card_id) FROM review_history");
        if (stmt && stmt.step()) s.learned_unique_cards = stmt.column_int(0);
    }

    // 连续打卡：从今天往前数，连续有 review_history 的天数
    int64_t now = std::time(nullptr);
    int64_t today = local_midnight(now);
    int streak = 0;
    {
        auto stmt = db_.prepare(
            "SELECT COUNT(*) FROM review_history "
            "WHERE reviewed_at >= ? AND reviewed_at < ?");
        if (stmt) {
            for (int i = 0; i < 365; ++i) {
                int64_t day_start = today - static_cast<int64_t>(i) * 86400;
                int64_t day_end = day_start + 86400;
                stmt.reset();
                stmt.bind(1, day_start);
                stmt.bind(2, day_end);
                if (!stmt.step()) break;
                int n = stmt.column_int(0);
                if (n == 0) {
                    if (i == 0) {
                        // 今天没复习也不打断（明天再复习连续仍 = 1）
                        // 但当前 streak 视为 0
                    }
                    break;
                }
                streak++;
            }
        }
    }
    s.streak_days = streak;

    // 最近 30 天活跃天数
    {
        auto stmt = db_.prepare(
            "SELECT COUNT(DISTINCT date(reviewed_at, 'unixepoch', 'localtime')) "
            "FROM review_history WHERE reviewed_at >= ?");
        if (stmt) {
            stmt.bind(1, static_cast<int64_t>(today - 30LL * 86400));
            if (stmt.step()) s.active_days_30 = stmt.column_int(0);
        }
    }

    return s;
}

Result<std::vector<TopicProgress>> StatsService::per_topic() {
    std::vector<TopicProgress> out;
    int64_t now = std::time(nullptr);

    auto stmt = db_.prepare(
        "SELECT t.id, t.name, t.title, "
        "       (SELECT COUNT(*) FROM card WHERE topic_id = t.id) AS total_cards, "
        "       (SELECT COUNT(DISTINCT rh.card_id) FROM review_history rh "
        "         JOIN card c ON c.id = rh.card_id WHERE c.topic_id = t.id) AS learned, "
        "       (SELECT COUNT(*) FROM review_history rh "
        "         JOIN card c ON c.id = rh.card_id "
        "         WHERE c.topic_id = t.id AND rh.score >= 3) AS correct_total, "
        "       (SELECT COUNT(*) FROM review_history rh "
        "         JOIN card c ON c.id = rh.card_id "
        "         WHERE c.topic_id = t.id) AS review_total, "
        "       (SELECT COUNT(*) FROM review r "
        "         JOIN card c ON c.id = r.card_id "
        "         WHERE c.topic_id = t.id AND r.next_review <= ? "
        "           AND r.suspended = 0) AS due_today "
        "FROM topic t ORDER BY t.name");
    if (!stmt) {
        return make_err<std::vector<TopicProgress>>(E::kDbPrepareFailed,
            "prepare per_topic failed");
    }
    stmt.bind(1, now);

    while (stmt.step()) {
        TopicProgress p;
        p.topic_id     = stmt.column_int64(0);
        p.topic_name   = stmt.column_text(1);
        p.title        = stmt.column_text(2);
        p.total_cards  = stmt.column_int(3);
        p.learned_cards = stmt.column_int(4);
        int correct_total = stmt.column_int(5);
        int review_total = stmt.column_int(6);
        p.correct_cards = correct_total;   // 简化：直接用对的次数
        p.due_today    = stmt.column_int(7);
        if (review_total > 0) {
            p.accuracy = static_cast<double>(correct_total) / review_total;
        }
        out.push_back(std::move(p));
    }
    return out;
}

Result<std::vector<DailyCount>> StatsService::daily_counts(int days) {
    if (days <= 0) days = 90;

    int64_t now = std::time(nullptr);
    int64_t today = local_midnight(now);
    int64_t start = today - static_cast<int64_t>(days - 1) * 86400;

    // 一次性查每天计数
    std::unordered_map<std::string, int> date_to_count;
    {
        auto stmt = db_.prepare(
            "SELECT date(reviewed_at, 'unixepoch', 'localtime') AS d, COUNT(*) "
            "FROM review_history WHERE reviewed_at >= ? "
            "GROUP BY d");
        if (!stmt) return make_err<std::vector<DailyCount>>(
            E::kDbPrepareFailed, "prepare daily_counts failed");
        stmt.bind(1, start);
        while (stmt.step()) {
            date_to_count[stmt.column_text(0)] = stmt.column_int(1);
        }
    }

    // 填充每一天（包含 0 计数）
    std::vector<DailyCount> out;
    out.reserve(days);
    for (int i = 0; i < days; ++i) {
        int64_t day_ts = start + static_cast<int64_t>(i) * 86400;
        DailyCount d;
        d.date = fmt_date(day_ts);
        auto it = date_to_count.find(d.date);
        d.count = it == date_to_count.end() ? 0 : it->second;
        out.push_back(std::move(d));
    }
    return out;
}

Result<std::vector<WeakCard>> StatsService::weak_cards(int recent_n, int top_k) {
    if (recent_n <= 0) recent_n = 50;
    if (top_k <= 0) top_k = 10;

    auto stmt = db_.prepare(
        "WITH recent AS ( "
        "  SELECT card_id, score FROM review_history "
        "  ORDER BY reviewed_at DESC LIMIT ? "
        ") "
        "SELECT card.id, card.question, topic.name, "
        "       SUM(CASE WHEN recent.score < 3 THEN 1 ELSE 0 END) AS wrong, "
        "       COUNT(*) AS total "
        "FROM recent "
        "JOIN card ON card.id = recent.card_id "
        "JOIN topic ON topic.id = card.topic_id "
        "GROUP BY card.id "
        "HAVING wrong > 0 "
        "ORDER BY wrong DESC, total DESC "
        "LIMIT ?");
    if (!stmt) return make_err<std::vector<WeakCard>>(
        E::kDbPrepareFailed, "prepare weak_cards failed");

    stmt.bind(1, recent_n);
    stmt.bind(2, top_k);

    std::vector<WeakCard> out;
    while (stmt.step()) {
        WeakCard w;
        w.card_id      = stmt.column_int64(0);
        w.question     = stmt.column_text(1);
        w.topic_name   = stmt.column_text(2);
        w.wrong_count  = stmt.column_int(3);
        w.total_recent = stmt.column_int(4);
        out.push_back(std::move(w));
    }
    return out;
}

}  // namespace bagu::service
