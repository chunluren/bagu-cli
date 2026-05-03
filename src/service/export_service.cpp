#include "service/export_service.h"

#include <spdlog/spdlog.h>

#include "db/card_dao.h"
#include "db/topic_dao.h"

namespace bagu::service {

namespace {

/// 转义单个字段内容供 TAB-分隔 txt 使用：
/// 1. TAB → 4 空格（Anki 列分隔符冲突）
/// 2. CRLF / LF → `<br>`（Anki HTML 渲染会换行）
/// 3. 字符串内已有的 `<br>` 字面量保留（合法 HTML）
std::string escape_field(const std::string& s) {
    std::string out;
    out.reserve(s.size() + 16);
    for (size_t i = 0; i < s.size(); ++i) {
        char c = s[i];
        if (c == '\t') {
            out.append("    ");
        } else if (c == '\r') {
            // 跳过；下一个 \n 会处理（或者 \r 单独出现也变 <br>）
            if (i + 1 < s.size() && s[i + 1] == '\n') continue;
            out.append("<br>");
        } else if (c == '\n') {
            out.append("<br>");
        } else {
            out.push_back(c);
        }
    }
    return out;
}

}  // namespace

Result<AnkiExportSummary> ExportService::export_anki(
    const AnkiExportOptions& opts, std::ostream& out) {

    AnkiExportSummary sum;

    // Anki "Notes in Plain Text" 标头（可选；Anki 能自动识别但写上更友好）
    out << "#separator:tab\n";
    out << "#html:true\n";
    out << "#columns:Front\tBack\tTags\n";
    out << "#tags column:3\n";

    db::TopicDao tdao(db_);
    db::CardDao cdao(db_);

    auto write_topic_cards = [&](const db::Topic& t) -> Result<void> {
        auto cards_r = cdao.find_by_topic(t.id);
        if (cards_r.is_err()) return Result<void>(cards_r.error());

        for (const auto& c : cards_r.value()) {
            sum.total_cards++;
            if (c.card_type != "qa" && !opts.include_section_cards) {
                sum.skipped++;
                continue;
            }
            // tag：bagu + topic name + （tags 字段拆空格）
            std::string tags = "bagu " + t.name;
            if (!c.tags.empty()) {
                tags.push_back(' ');
                // 把 tags 字段里的 tab/逗号替换成空格
                for (char ch : c.tags) {
                    tags.push_back((ch == ',' || ch == '\t') ? ' ' : ch);
                }
            }
            out << escape_field(c.question) << '\t'
                << escape_field(c.answer)   << '\t'
                << escape_field(tags)       << '\n';
            sum.written++;
        }
        return Result<void>::ok();
    };

    if (opts.topic.empty()) {
        auto all_r = tdao.find_all();
        if (all_r.is_err()) return Result<AnkiExportSummary>(all_r.error());
        for (const auto& t : all_r.value()) {
            auto r = write_topic_cards(t);
            if (r.is_err()) return Result<AnkiExportSummary>(r.error());
        }
    } else {
        auto t_r = tdao.find_by_name(opts.topic);
        if (t_r.is_err()) return Result<AnkiExportSummary>(t_r.error());
        if (!t_r.value().has_value()) {
            return make_err<AnkiExportSummary>(E::kTopicNotFound,
                "topic not found", "name=" + opts.topic);
        }
        auto r = write_topic_cards(*t_r.value());
        if (r.is_err()) return Result<AnkiExportSummary>(r.error());
    }

    spdlog::info("export_anki: total={} written={} skipped={}",
        sum.total_cards, sum.written, sum.skipped);
    return sum;
}

namespace {

/// CSV 字段转义（RFC 4180）：含 , " CR LF 时用 " 包，内部 " 转 ""
std::string csv_escape(const std::string& s) {
    bool need_quote = false;
    for (char c : s) {
        if (c == ',' || c == '"' || c == '\n' || c == '\r') { need_quote = true; break; }
    }
    if (!need_quote) return s;
    std::string out;
    out.reserve(s.size() + 4);
    out.push_back('"');
    for (char c : s) {
        if (c == '"') out.push_back('"');
        out.push_back(c);
    }
    out.push_back('"');
    return out;
}

}  // namespace

Result<AnkiExportSummary> ExportService::export_csv(
    const CsvExportOptions& opts, std::ostream& out) {

    AnkiExportSummary sum;

    // RFC 4180 头一行 + CRLF
    out << "id,topic,chapter,question,answer,tags,card_type\r\n";

    db::TopicDao tdao(db_);
    db::CardDao cdao(db_);

    auto write_topic = [&](const db::Topic& t) -> Result<void> {
        auto cards_r = cdao.find_by_topic(t.id);
        if (cards_r.is_err()) return Result<void>(cards_r.error());

        for (const auto& c : cards_r.value()) {
            sum.total_cards++;
            if (c.card_type != "qa" && !opts.include_section_cards) {
                sum.skipped++;
                continue;
            }
            std::string chapter_str;  // 简化：不 join chapter 表，留空
            out << c.id << ','
                << csv_escape(t.name) << ','
                << csv_escape(chapter_str) << ','
                << csv_escape(c.question) << ','
                << csv_escape(c.answer) << ','
                << csv_escape(c.tags) << ','
                << csv_escape(c.card_type) << "\r\n";
            sum.written++;
        }
        return Result<void>::ok();
    };

    if (opts.topic.empty()) {
        auto all_r = tdao.find_all();
        if (all_r.is_err()) return Result<AnkiExportSummary>(all_r.error());
        for (const auto& t : all_r.value()) {
            auto r = write_topic(t);
            if (r.is_err()) return Result<AnkiExportSummary>(r.error());
        }
    } else {
        auto t_r = tdao.find_by_name(opts.topic);
        if (t_r.is_err()) return Result<AnkiExportSummary>(t_r.error());
        if (!t_r.value().has_value()) {
            return make_err<AnkiExportSummary>(E::kTopicNotFound,
                "topic not found", "name=" + opts.topic);
        }
        auto r = write_topic(*t_r.value());
        if (r.is_err()) return Result<AnkiExportSummary>(r.error());
    }

    spdlog::info("export_csv: total={} written={} skipped={}",
        sum.total_cards, sum.written, sum.skipped);
    return sum;
}

}  // namespace bagu::service
