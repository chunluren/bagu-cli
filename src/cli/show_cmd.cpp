#include "cli/show_cmd.h"

#include <iostream>

#include "bagu/error.h"
#include "db/card_dao.h"
#include "db/chapter_dao.h"
#include "db/database.h"
#include "db/topic_dao.h"
#include "util/path.h"

namespace bagu::cli {

int run_show(int64_t card_id) {
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

    db::CardDao card_dao(db);
    auto c_r = card_dao.find_by_id(card_id);
    if (c_r.is_err()) {
        std::cerr << "Error: " << c_r.error().message << "\n";
        return to_exit_code(c_r.error().code);
    }
    if (!c_r.value().has_value()) {
        std::cerr << "Error: 卡片不存在: id=" << card_id << " (E"
                  << static_cast<int>(E::kCardNotFound) << ")\n";
        return to_exit_code(static_cast<int>(E::kCardNotFound));
    }
    auto& card = *c_r.value();

    // 关联 topic / chapter 名
    db::TopicDao topic_dao(db);
    db::ChapterDao chapter_dao(db);
    auto topic = topic_dao.find_by_id(card.topic_id);
    std::string topic_name = "?", chapter_name = "(no chapter)";
    if (topic.is_ok() && topic.value().has_value()) {
        topic_name = topic.value()->name;
    }
    if (card.chapter_id.has_value()) {
        // 简单查一下章节名（不通过 dao 因为没有 find_by_id）
        auto stmt = db.prepare("SELECT chapter_no, name FROM chapter WHERE id = ?");
        if (stmt) {
            stmt.bind(1, *card.chapter_id);
            if (stmt.step()) {
                int no = stmt.column_int(0);
                std::string n = stmt.column_text(1);
                chapter_name = "第 " + std::to_string(no) + " 章 " + n;
            }
        }
    }

    std::cout << "\n========== Card #" << card.id << " ==========\n";
    std::cout << "Topic:    " << topic_name << "\n";
    std::cout << "Chapter:  " << chapter_name << "\n";
    std::cout << "Type:     " << card.card_type << "\n";
    std::cout << "Source:   line " << card.source_line << "\n";
    std::cout << "\nQ: " << card.question << "\n";
    std::cout << "\nA:\n" << card.answer << "\n";
    std::cout << "\n";

    return 0;
}

}  // namespace bagu::cli
