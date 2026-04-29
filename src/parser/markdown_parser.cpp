#include "parser/markdown_parser.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <fstream>
#include <regex>
#include <sstream>

namespace bagu::parser {

namespace fs = std::filesystem;

namespace {

// ---------- 字符串工具 ----------

std::string trim(std::string_view s) {
    size_t start = 0;
    while (start < s.size() && std::isspace(static_cast<unsigned char>(s[start]))) ++start;
    size_t end = s.size();
    while (end > start && std::isspace(static_cast<unsigned char>(s[end - 1]))) --end;
    return std::string(s.substr(start, end - start));
}

std::vector<std::string> split_lines(std::string_view content) {
    std::vector<std::string> lines;
    size_t start = 0;
    for (size_t i = 0; i <= content.size(); ++i) {
        if (i == content.size() || content[i] == '\n') {
            lines.emplace_back(content.substr(start, i - start));
            start = i + 1;
        }
    }
    return lines;
}

// ---------- 正则 ----------

// "## 第 N 章 XXX" 或 "## 第N章XXX"，提取 N 和 XXX
// 也允许 "## XXX"（无章号，归到 chapter_no=0）
// (?!#) 负向后查避免匹配到 ###
const std::regex kRegexH2Chapter(
    R"(^##(?!#)\s*(?:第\s*(\d+)\s*章\s*)?(.+?)\s*$)");

// "### N.M XXX" 提取 N、M、XXX
// (?!#) 避免匹配到 ####
const std::regex kRegexH3Section(
    R"(^###(?!#)\s*(\d+)\.(\d+)\s+(.+?)\s*$)");

// 题库 Q/A 行的两种格式：
//   1. **Q1. 问题？** 答案     （编号格式，redis/cpp-net）
//   2. **Q: 问题？** 答案       （冒号格式，mysql 章末追问）
// 注意 (.+?) 是非贪婪，匹配到最近的 **
const std::regex kRegexQa(
    R"(^\*\*Q(?:\d+\.|:)\s*(.+?)\*\*\s*(.*)$)");

}  // namespace

// ============================================================================
// Topic name 推断
// ============================================================================

std::string topic_name_from_filename(const std::string& filename) {
    fs::path p(filename);
    std::string stem = p.stem().string();

    // 转小写
    std::transform(stem.begin(), stem.end(), stem.begin(),
        [](unsigned char c) { return std::tolower(c); });

    // 移除常见后缀关键字（-interview / 八股文档）
    auto remove_suffix = [&](const std::string& suf) {
        if (stem.size() > suf.size() &&
            stem.compare(stem.size() - suf.size(), suf.size(), suf) == 0) {
            stem.erase(stem.size() - suf.size());
        }
    };
    remove_suffix("-interview");

    // 中文 "八股文档" / "八股" 等
    auto erase_substr = [&](const std::string& s) {
        size_t pos = stem.find(s);
        while (pos != std::string::npos) {
            stem.erase(pos, s.size());
            pos = stem.find(s);
        }
    };
    erase_substr("八股文档");
    erase_substr("八股");
    erase_substr(" ");

    // 去掉首尾空白和连字符
    while (!stem.empty() && (stem.front() == '-' || stem.front() == '_'))
        stem.erase(stem.begin());
    while (!stem.empty() && (stem.back() == '-' || stem.back() == '_'))
        stem.pop_back();

    return stem.empty() ? "untitled" : stem;
}

// ============================================================================
// MarkdownParser 实现
// ============================================================================

Result<ParsedDocument> MarkdownParser::parse_file(const fs::path& path) {
    std::ifstream in(path);
    if (!in) {
        return make_err<ParsedDocument>(E::kFileReadFailed,
            "无法读取文件", "path=" + path.string());
    }
    std::stringstream ss;
    ss << in.rdbuf();
    return parse_string(ss.str(), path.filename().string());
}

Result<ParsedDocument> MarkdownParser::parse_string(std::string_view content,
                                                  std::string filename) {
    ParsedDocument doc;
    if (!filename.empty()) {
        doc.topic_name = topic_name_from_filename(filename);
    }

    auto lines = split_lines(content);

    // 状态
    bool in_code_block = false;
    int current_top_chapter = 0;        // 当前 ## 章号

    // 收集"小节正文" buffer（从 ### 标题开始到下一个标题之前）
    std::optional<ParsedCard> section_card_buf;
    std::ostringstream section_body;

    auto flush_section_card = [&]() {
        if (!section_card_buf.has_value()) return;
        std::string body = trim(section_body.str());
        if (!body.empty()) {
            section_card_buf->answer = std::move(body);
            doc.cards.push_back(std::move(*section_card_buf));
        }
        section_card_buf.reset();
        section_body.str({});
        section_body.clear();
    };

    for (size_t i = 0; i < lines.size(); ++i) {
        const std::string& line = lines[i];
        int line_no = static_cast<int>(i + 1);

        // 代码块边界
        if (line.size() >= 3 && line.substr(0, 3) == "```") {
            in_code_block = !in_code_block;
            // 把代码块行原样收入 body
            if (section_card_buf.has_value()) section_body << line << "\n";
            continue;
        }
        if (in_code_block) {
            if (section_card_buf.has_value()) section_body << line << "\n";
            continue;
        }

        // # 顶级标题（仅取第一个作为 doc.title）
        if (line.size() >= 2 && line[0] == '#' && line[1] != '#') {
            if (doc.title.empty()) {
                doc.title = trim(line.substr(1));
            }
            continue;
        }

        std::smatch m;

        // ## 第 N 章 — 顶级章节
        if (std::regex_match(line, m, kRegexH2Chapter)) {
            flush_section_card();

            ParsedChapter ch;
            ch.level = 2;
            ch.source_line = line_no;
            if (m[1].matched) {
                ch.chapter_no = std::stoi(m[1].str());
            } else {
                // ## XXX 无章号 → 用递增的负数表示，避免冲突
                ch.chapter_no = -static_cast<int>(doc.chapters.size() + 1);
            }
            ch.name = trim(m[2].str());

            current_top_chapter = ch.chapter_no;
            doc.chapters.push_back(std::move(ch));
            continue;
        }

        // ### N.M XX — 小节
        if (std::regex_match(line, m, kRegexH3Section)) {
            flush_section_card();

            int n = std::stoi(m[1].str());
            int sub = std::stoi(m[2].str());
            int section_no = n * 1000 + sub;
            std::string name = trim(m[3].str());

            ParsedChapter ch;
            ch.level = 3;
            ch.chapter_no = section_no;
            ch.parent_chapter_no = current_top_chapter;
            ch.name = std::to_string(n) + "." + std::to_string(sub) + " " + name;
            ch.source_line = line_no;
            doc.chapters.push_back(ch);
            (void)section_no;

            // 同时开一张 section card（标题做问题，正文做答案）
            ParsedCard card;
            card.question = name;
            card.chapter_no = current_top_chapter;
            card.card_type = "section";
            card.source_line = line_no;
            section_card_buf = std::move(card);
            continue;
        }

        // **Q数字.** 问题 — 题库 Q/A
        if (std::regex_match(line, m, kRegexQa)) {
            std::string question = trim(m[1].str());

            // 答案：本行剩余 + 后续行直到下一个 ** 或空行 或下一个 Q
            std::ostringstream ans;
            std::string first_rest = trim(m[2].str());
            if (!first_rest.empty()) ans << first_rest << "\n";

            size_t j = i + 1;
            while (j < lines.size()) {
                const std::string& next = lines[j];
                std::string trimmed = trim(next);

                // 遇到下一个 Q / 标题 / 空行（连续两个）→ 结束
                if (trimmed.empty()) {
                    // 单空行可以接受（段落分隔），但连续两个空行视为结束
                    if (j + 1 < lines.size() && trim(lines[j + 1]).empty()) {
                        break;
                    }
                    ans << "\n";
                    ++j;
                    continue;
                }
                if (trimmed.size() >= 3 && trimmed.substr(0, 3) == "**Q") break;
                if (!trimmed.empty() && trimmed[0] == '#') break;

                ans << next << "\n";
                ++j;
            }

            ParsedCard card;
            card.question = question;
            card.answer = trim(ans.str());
            card.chapter_no = current_top_chapter;
            card.card_type = "qa";
            card.source_line = line_no;
            doc.cards.push_back(std::move(card));

            i = j - 1;  // 跳过已消耗的行
            continue;
        }

        // 普通正文 — 累积到当前 section card buffer
        if (section_card_buf.has_value()) {
            section_body << line << "\n";
        }
    }

    flush_section_card();

    return doc;
}

}  // namespace bagu::parser
