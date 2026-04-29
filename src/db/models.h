#pragma once

#include <cstdint>
#include <optional>
#include <string>

namespace bagu::db {

/// 主题（一份 markdown 文档 = 一个 topic）
struct Topic {
    int64_t id = 0;
    std::string name;        // 'mysql' / 'redis'，作为命令参数
    std::string title;       // 'MySQL 八股文档'，显示用
    std::string file_path;   // 源 md 绝对路径
    std::string file_hash;   // 内容 SHA256，判重
    int64_t imported_at = 0; // unix timestamp
    int64_t updated_at = 0;
};

/// 章节（## 第 N 章 / ### N.M）
struct Chapter {
    int64_t id = 0;
    int64_t topic_id = 0;
    std::string name;        // '索引'
    int chapter_no = 0;      // 3
    std::optional<int64_t> parent_id;  // NULL = 顶级章节
};

/// 卡片（最小复习单元）
struct Card {
    int64_t id = 0;
    int64_t topic_id = 0;
    std::optional<int64_t> chapter_id;
    std::string question;
    std::string answer;
    int difficulty = 2;       // 1-3
    std::string tags;         // JSON array as string
    int source_line = 0;      // 在源 md 中的行号
    std::string card_type = "qa";   // 'qa' / 'section' / 'code'
    int64_t created_at = 0;
    int64_t updated_at = 0;
};

}  // namespace bagu::db
