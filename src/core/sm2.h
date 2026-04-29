#pragma once

#include <cstdint>

namespace bagu::core {

/// 单张卡片的复习状态（SM-2 算法所需）
struct ReviewState {
    int interval_days = 0;     // 当前复习间隔（天）
    double ease_factor = 2.5;  // SM-2 难度因子
    int repetitions = 0;       // 连续答对次数（score >= 3）
};

/// 单次评分（SuperMemo SM-2 标准分级）
enum class Score : int {
    BlankOut = 0,         // 完全忘记
    Wrong = 1,            // 答错，但有印象
    AlmostRight = 2,      // 答错，但记起来了
    Hesitant = 3,         // 答对，需思考
    OkayHesitant = 4,     // 答对，稍犹豫
    Perfect = 5,          // 完美
};

class SM2Algorithm {
public:
    /// SM-2 评分更新规则
    ///
    /// score < 3 → 失败：repetitions=0, interval=1, ease 减
    /// score == 3 → 成功但难：repetitions++, interval 按 ease 增长
    /// score == 4 → 成功：同 3
    /// score == 5 → 成功且容易：repetitions++, ease 加
    ///
    /// @param prev 当前状态
    /// @param score 0-5 评分
    /// @return 更新后的状态
    static ReviewState update(ReviewState prev, int score);

    /// 计算下次复习时间戳（unix seconds）
    /// @param state 更新后的状态
    /// @param now_ts 当前时间戳
    static int64_t next_review_time(const ReviewState& state, int64_t now_ts);

    /// 获取 ease factor 下限（防止 ease 无限下降）
    static constexpr double min_ease_factor() { return 1.3; }
};

}  // namespace bagu::core
