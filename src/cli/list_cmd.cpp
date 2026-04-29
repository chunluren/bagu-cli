#include "cli/list_cmd.h"

#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>

#include "bagu/error.h"
#include "db/card_dao.h"
#include "db/chapter_dao.h"
#include "db/database.h"
#include "db/migrations.h"
#include "db/topic_dao.h"
#include "util/path.h"

namespace bagu::cli {

namespace {

int open_db(db::Database& out_db) {
    auto db_path = util::default_db_path();
    if (!std::filesystem::exists(db_path)) {
        std::cerr << "Error: 数据库不存在 (E"
                  << static_cast<int>(E::kDbNotFound) << ")\n\n"
                  << "  path: " << db_path.string() << "\n\n"
                  << "提示：请先 `bagu init` 并 `bagu import <path>`\n";
        return to_exit_code(static_cast<int>(E::kDbNotFound));
    }

    auto r = db::Database::open(db_path);
    if (r.is_err()) {
        std::cerr << "Error: " << r.error().message << "\n";
        return to_exit_code(r.error().code);
    }
    out_db = std::move(r.value());
    return 0;
}

int list_all_topics(db::Database& db) {
    db::TopicDao topic_dao(db);
    db::ChapterDao chapter_dao(db);
    db::CardDao card_dao(db);

    auto topics_r = topic_dao.find_all();
    if (topics_r.is_err()) {
        std::cerr << "Error: " << topics_r.error().message << "\n";
        return to_exit_code(topics_r.error().code);
    }
    auto& topics = topics_r.value();

    if (topics.empty()) {
        std::cout << "(no topics imported yet)\n\n"
                  << "提示：用 `bagu import <path>` 导入 markdown 文档\n";
        return 0;
    }

    // 表头
    std::cout << "\n";
    std::cout << "  " << std::left
              << std::setw(20) << "TOPIC"
              << std::right
              << std::setw(8) << "CARDS"
              << std::setw(12) << "CHAPTERS"
              << "  TITLE\n";
    std::cout << "  " << std::string(64, '-') << "\n";

    int total_cards = 0;
    for (const auto& t : topics) {
        auto cards = card_dao.count_by_topic(t.id).value_or(0);
        auto chapters = chapter_dao.count_by_topic(t.id).value_or(0);
        total_cards += static_cast<int>(cards);

        std::cout << "  " << std::left
                  << std::setw(20) << t.name
                  << std::right
                  << std::setw(8) << cards
                  << std::setw(12) << chapters
                  << "  " << t.title << "\n";
    }
    std::cout << "  " << std::string(64, '-') << "\n";
    std::cout << "  Total: " << topics.size() << " topics, "
              << total_cards << " cards\n\n";
    return 0;
}

int list_topic_detail(db::Database& db, const std::string& topic_name) {
    db::TopicDao topic_dao(db);
    db::ChapterDao chapter_dao(db);

    auto t_r = topic_dao.find_by_name(topic_name);
    if (t_r.is_err()) {
        std::cerr << "Error: " << t_r.error().message << "\n";
        return to_exit_code(t_r.error().code);
    }
    if (!t_r.value().has_value()) {
        std::cerr << "Error: 主题不存在: " << topic_name << " (E"
                  << static_cast<int>(E::kTopicNotFound) << ")\n\n"
                  << "提示：用 `bagu list` 看所有主题\n";
        return to_exit_code(static_cast<int>(E::kTopicNotFound));
    }
    auto& topic = *t_r.value();

    auto chapters_r = chapter_dao.find_by_topic(topic.id);
    if (chapters_r.is_err()) {
        std::cerr << "Error: " << chapters_r.error().message << "\n";
        return to_exit_code(chapters_r.error().code);
    }

    // 卡片数（按章节统计）
    auto cards_r = db::CardDao(db).find_by_topic(topic.id);
    if (cards_r.is_err()) {
        std::cerr << "Error: " << cards_r.error().message << "\n";
        return to_exit_code(cards_r.error().code);
    }
    std::map<int64_t, int> chapter_card_count;
    for (const auto& c : cards_r.value()) {
        if (c.chapter_id) chapter_card_count[*c.chapter_id]++;
    }

    int total_cards = static_cast<int>(cards_r.value().size());
    std::cout << "\n  " << topic.title << "\n"
              << "  " << total_cards << " cards · " << chapters_r.value().size()
              << " chapters\n\n";

    // 顶级章节先列（按 chapter_no 排序），子章节缩进
    auto& chapters = chapters_r.value();
    std::vector<const db::Chapter*> top;
    std::map<int64_t, std::vector<const db::Chapter*>> by_parent;
    for (auto& c : chapters) {
        if (c.parent_id.has_value()) {
            by_parent[*c.parent_id].push_back(&c);
        } else {
            top.push_back(&c);
        }
    }
    std::sort(top.begin(), top.end(), [](auto a, auto b) {
        return a->chapter_no < b->chapter_no;
    });

    for (size_t i = 0; i < top.size(); ++i) {
        bool last = (i + 1 == top.size());
        const db::Chapter* ch = top[i];
        int direct = chapter_card_count[ch->id];
        std::cout << "  " << (last ? "└── " : "├── ")
                  << "第 " << ch->chapter_no << " 章 " << ch->name;
        if (direct > 0) std::cout << "  (" << direct << " cards)";
        std::cout << "\n";

        auto& subs = by_parent[ch->id];
        std::sort(subs.begin(), subs.end(), [](auto a, auto b) {
            return a->chapter_no < b->chapter_no;
        });
        std::string indent = last ? "      " : "  │   ";
        for (size_t j = 0; j < subs.size(); ++j) {
            bool sub_last = (j + 1 == subs.size());
            std::cout << indent << (sub_last ? "└── " : "├── ")
                      << subs[j]->name << "\n";
        }
    }
    std::cout << "\n";
    return 0;
}

}  // namespace

int run_list(const std::string& topic) {
    db::Database db;
    if (int rc = open_db(db); rc != 0) return rc;

    if (topic.empty()) return list_all_topics(db);
    return list_topic_detail(db, topic);
}

}  // namespace bagu::cli
