#include "cli/search_cmd.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <unistd.h>
#include <unordered_map>

#include "bagu/error.h"
#include "db/card_dao.h"
#include "db/database.h"
#include "db/topic_dao.h"
#include "util/path.h"

namespace bagu::cli {

namespace {

bool is_tty() {
    return ::isatty(STDOUT_FILENO);
}

// ANSI 颜色（仅 TTY 启用）
struct Colors {
    const char* bold;
    const char* dim;
    const char* yellow;
    const char* cyan;
    const char* green;
    const char* reset;
};

Colors get_colors() {
    if (is_tty()) {
        return {"\033[1m", "\033[2m", "\033[33m", "\033[36m", "\033[32m", "\033[0m"};
    }
    return {"", "", "", "", "", ""};
}

/// 把 pos 调整到 UTF-8 字符边界（向后找）
size_t utf8_align_forward(const std::string& s, size_t pos) {
    while (pos < s.size() &&
           (static_cast<unsigned char>(s[pos]) & 0xC0) == 0x80) {
        ++pos;
    }
    return pos;
}

/// 把 pos 调整到 UTF-8 字符边界（向前找）
size_t utf8_align_backward(const std::string& s, size_t pos) {
    while (pos > 0 &&
           (static_cast<unsigned char>(s[pos]) & 0xC0) == 0x80) {
        --pos;
    }
    return pos;
}

/// 在 text 中找 keyword，返回截取上下文（前后最多 max_ctx 字节），把 keyword 高亮
/// 大小写不敏感（仅 ASCII keyword 按字节匹配；UTF-8 边界对齐避免半字符）
std::string highlight_context(const std::string& text,
                             const std::string& keyword,
                             const Colors& C,
                             size_t max_ctx = 60) {
    if (keyword.empty() || text.empty()) {
        size_t end = std::min(text.size(), max_ctx * 2);
        end = utf8_align_backward(text, end);
        return text.substr(0, end);
    }

    // 大小写不敏感的查找（仅 ASCII）
    auto lower = [](char c) { return std::tolower(static_cast<unsigned char>(c)); };
    auto find_ci = [&](size_t from) -> size_t {
        for (size_t i = from; i + keyword.size() <= text.size(); ++i) {
            bool match = true;
            for (size_t j = 0; j < keyword.size(); ++j) {
                if (lower(text[i + j]) != lower(keyword[j])) { match = false; break; }
            }
            if (match) return i;
        }
        return std::string::npos;
    };

    size_t pos = find_ci(0);
    std::string out;

    if (pos == std::string::npos) {
        size_t end = std::min(text.size(), max_ctx * 2);
        end = utf8_align_backward(text, end);
        out = text.substr(0, end);
        if (text.size() > end) out += "...";
        for (auto& ch : out) if (ch == '\n' || ch == '\r') ch = ' ';
        return out;
    }

    size_t start = pos > max_ctx ? pos - max_ctx : 0;
    size_t end = std::min(text.size(), pos + keyword.size() + max_ctx);

    // UTF-8 对齐：start 向后到下一个完整字符开始；end 向前到完整字符末尾
    start = utf8_align_forward(text, start);
    end = utf8_align_backward(text, end);

    if (start > 0) out += "...";
    out.append(text, start, pos - start);
    out += C.yellow;
    out += C.bold;
    out.append(text, pos, keyword.size());
    out += C.reset;
    out.append(text, pos + keyword.size(), end - pos - keyword.size());
    if (end < text.size()) out += "...";

    for (auto& ch : out) if (ch == '\n' || ch == '\r') ch = ' ';
    return out;
}

}  // namespace

int run_search(const std::string& keyword, const std::string& topic, int limit) {
    if (keyword.empty()) {
        std::cerr << "Error: 关键词不能为空\n";
        return to_exit_code(static_cast<int>(E::kArgRequired));
    }
    if (limit <= 0) limit = 20;

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

    // 解析 topic 限定
    int64_t topic_id = 0;
    if (!topic.empty()) {
        auto t_r = db::TopicDao(db).find_by_name(topic);
        if (t_r.is_err()) {
            std::cerr << "Error: " << t_r.error().message << "\n";
            return to_exit_code(t_r.error().code);
        }
        if (!t_r.value().has_value()) {
            std::cerr << "Error: 主题不存在: " << topic << "\n";
            return to_exit_code(static_cast<int>(E::kTopicNotFound));
        }
        topic_id = t_r.value()->id;
    }

    // 执行 FTS 搜索
    auto t0 = std::chrono::steady_clock::now();
    auto r = db::CardDao(db).search(keyword, topic_id, limit);
    auto t1 = std::chrono::steady_clock::now();
    int ms = static_cast<int>(
        std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0).count());

    if (r.is_err()) {
        std::cerr << "Error: " << r.error().message << "\n  "
                  << r.error().detail << "\n";
        return to_exit_code(r.error().code);
    }
    auto& cards = r.value();

    if (cards.empty()) {
        std::cout << "No results for \"" << keyword << "\""
                  << (topic.empty() ? "" : " in " + topic)
                  << " (" << ms << "ms)\n";
        return 0;
    }

    // 取每张卡片的 topic name 缓存
    db::TopicDao topic_dao(db);
    std::unordered_map<int64_t, std::string> topic_cache;
    auto get_topic = [&](int64_t tid) -> std::string {
        if (auto it = topic_cache.find(tid); it != topic_cache.end()) return it->second;
        auto tr = topic_dao.find_by_id(tid);
        std::string name = (tr.is_ok() && tr.value().has_value()) ? tr.value()->name : "?";
        topic_cache[tid] = name;
        return name;
    };

    // 章节名查询（懒查）
    auto get_chapter = [&](std::optional<int64_t> chid) -> std::string {
        if (!chid.has_value()) return {};
        auto stmt = db.prepare("SELECT chapter_no, name FROM chapter WHERE id = ?");
        if (!stmt) return {};
        stmt.bind(1, *chid);
        if (!stmt.step()) return {};
        int no = stmt.column_int(0);
        std::string n = stmt.column_text(1);
        if (no <= 0) return n;
        return "第 " + std::to_string(no) + " 章 " + n;
    };

    auto C = get_colors();
    std::cout << "\nFound " << cards.size() << " card"
              << (cards.size() == 1 ? "" : "s")
              << " for \"" << C.yellow << keyword << C.reset << "\""
              << " (" << ms << "ms)\n\n";

    for (const auto& c : cards) {
        std::string topic_name = get_topic(c.topic_id);
        std::string chapter = get_chapter(c.chapter_id);

        std::cout << "  " << C.dim << "[#" << c.id << "]" << C.reset
                  << " " << C.cyan << topic_name << C.reset;
        if (!chapter.empty()) std::cout << " " << C.dim << "/" << C.reset
                                        << " " << chapter;
        std::cout << "\n";

        std::cout << "      " << C.bold << c.question << C.reset << "\n";

        // 答案上下文（高亮关键词）
        std::string ctx = highlight_context(c.answer, keyword, C, 50);
        if (!ctx.empty()) {
            std::cout << "      " << C.dim << ctx << C.reset << "\n";
        }
        std::cout << "\n";
    }

    return 0;
}

}  // namespace bagu::cli
