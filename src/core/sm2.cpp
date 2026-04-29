#include "core/sm2.h"

#include <algorithm>
#include <cmath>

namespace bagu::core {

ReviewState SM2Algorithm::update(ReviewState s, int score) {
    // 输入裁剪
    if (score < 0) score = 0;
    if (score > 5) score = 5;

    if (score < 3) {
        // 失败：重置 repetitions，间隔设为 1 天
        s.repetitions = 0;
        s.interval_days = 1;
    } else {
        // 成功：按 repetitions 决定下次间隔
        if (s.repetitions == 0) {
            s.interval_days = 1;
        } else if (s.repetitions == 1) {
            s.interval_days = 6;
        } else {
            // round to nearest int
            double next = static_cast<double>(s.interval_days) * s.ease_factor;
            s.interval_days = static_cast<int>(std::lround(next));
        }
        s.repetitions++;
    }

    // ease factor 调整：q=score, EF' = EF + (0.1 - (5-q)*(0.08 + (5-q)*0.02))
    double q = static_cast<double>(score);
    double delta = 0.1 - (5 - q) * (0.08 + (5 - q) * 0.02);
    s.ease_factor = std::max(min_ease_factor(), s.ease_factor + delta);

    return s;
}

int64_t SM2Algorithm::next_review_time(const ReviewState& s, int64_t now_ts) {
    constexpr int64_t kSecondsPerDay = 24 * 60 * 60;
    int days = std::max(0, s.interval_days);
    return now_ts + static_cast<int64_t>(days) * kSecondsPerDay;
}

}  // namespace bagu::core
