#include "service/import_service.h"

#include <spdlog/spdlog.h>

#include <algorithm>
#include <cctype>
#include <chrono>
#include <ctime>
#include <unordered_map>
#include <unordered_set>

#include "db/card_dao.h"
#include "db/chapter_dao.h"
#include "db/topic_dao.h"
#include "parser/markdown_parser.h"
#include "util/hash.h"

namespace bagu::service {

namespace fs = std::filesystem;

namespace {

int64_t now_ts() {
    return static_cast<int64_t>(std::time(nullptr));
}

bool is_markdown_file(const fs::path& p) {
    if (!fs::is_regular_file(p)) return false;
    auto ext = p.extension().string();
    return ext == ".md" || ext == ".markdown";
}

std::vector<fs::path> collect_md_files(const fs::path& root) {
    std::vector<fs::path> out;
    if (is_markdown_file(root)) {
        out.push_back(root);
        return out;
    }
    if (!fs::is_directory(root)) return out;

    for (auto& entry : fs::recursive_directory_iterator(root,
            fs::directory_options::skip_permission_denied)) {
        if (is_markdown_file(entry.path())) {
            out.push_back(entry.path());
        }
    }
    std::sort(out.begin(), out.end());
    return out;
}

/// 把题面归一化为可哈希的稳定串：
/// - 去前后空白
/// - 内部连续空白合并为单空格
/// - lower-case（ASCII 范围；中文不变）
/// - 转义换行
std::string normalize_question(const std::string& q) {
    std::string out;
    out.reserve(q.size());
    bool last_was_space = true;  // 初始 true 让前导空白被吃掉
    for (char c : q) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc <= 0x20 || uc == '\t' || uc == '\n' || uc == '\r') {
            if (!last_was_space) { out.push_back(' '); last_was_space = true; }
        } else {
            // ASCII 字母小写
            if (uc >= 'A' && uc <= 'Z') uc = static_cast<unsigned char>(uc + 32);
            out.push_back(static_cast<char>(uc));
            last_was_space = false;
        }
    }
    if (!out.empty() && out.back() == ' ') out.pop_back();
    return out;
}

/// stable_key = sha256(topic_name + "::" + normalized_question)[:32]
std::string compute_stable_key(const std::string& topic_name,
                              const std::string& question) {
    std::string seed = topic_name + "::" + normalize_question(question);
    auto hex = util::sha256(seed);
    return hex.substr(0, 32);
}

}  // namespace

Result<ImportSummary> ImportService::import_path(const fs::path& path, Options opts) {
    if (!fs::exists(path)) {
        return make_err<ImportSummary>(E::kFileNotFound,
            "路径不存在", "path=" + path.string());
    }

    auto t0 = std::chrono::steady_clock::now();

    auto files = collect_md_files(path);
    if (files.empty()) {
        return make_err<ImportSummary>(E::kFileNotFound,
            "未找到 markdown 文件", "path=" + path.string());
    }

    ImportSummary summary;
    summary.total_files = static_cast<int>(files.size());

    for (const auto& f : files) {
        auto r = import_file(f, opts);
        if (!r.error.empty()) {
            summary.failed++;
        } else if (r.skipped) {
            summary.skipped++;
        } else {
            summary.succeeded++;
            summary.total_cards += r.cards_added;
        }
        summary.files.push_back(std::move(r));
    }

    auto t1 = std::chrono::steady_clock::now();
    summary.elapsed_seconds =
        std::chrono::duration<double>(t1 - t0).count();
    return summary;
}

FileImportResult ImportService::import_file(const fs::path& path, const Options& opts) {
    FileImportResult result;
    result.file_name = path.filename().string();

    // 1. 计算 SHA256
    auto hash_r = util::sha256_file(path);
    if (hash_r.is_err()) {
        result.error = hash_r.error().message;
        return result;
    }

    // 2. 解析 markdown
    parser::MarkdownParser parser;
    auto parse_r = parser.parse_file(path);
    if (parse_r.is_err()) {
        result.error = parse_r.error().message;
        return result;
    }
    auto& doc = parse_r.value();
    result.topic_name = doc.topic_name;

    // 3. 检查已有 topic
    db::TopicDao topic_dao(db_);
    auto existing_r = topic_dao.find_by_name(doc.topic_name);
    if (existing_r.is_err()) {
        result.error = existing_r.error().message;
        return result;
    }

    int64_t topic_id = 0;
    bool is_new = !existing_r.value().has_value();

    if (!is_new && existing_r.value()->file_hash == hash_r.value() && !opts.force) {
        result.skipped = true;
        result.topic_name = doc.topic_name;
        return result;
    }

    // 4. 事务：upsert topic / chapter / card
    auto txn = db_.begin();

    int64_t ts = now_ts();
    db::Topic topic;
    topic.name = doc.topic_name;
    topic.title = doc.title.empty() ? doc.topic_name : doc.title;
    topic.file_path = fs::absolute(path).string();
    topic.file_hash = hash_r.value();
    topic.imported_at = is_new ? ts : existing_r.value()->imported_at;
    topic.updated_at = ts;

    if (is_new) {
        auto ins_r = topic_dao.insert(topic);
        if (ins_r.is_err()) {
            result.error = ins_r.error().message;
            return result;
        }
        topic_id = ins_r.value();
    } else {
        topic_id = existing_r.value()->id;
        topic.id = topic_id;
        // 章节全删重建（章节没有复习状态可丢；保持简单）
        auto stmt = db_.prepare("DELETE FROM chapter WHERE topic_id = ?");
        if (!stmt) {
            result.error = "prepare DELETE chapter failed";
            return result;
        }
        stmt.bind(1, topic_id);
        if (stmt.execute() < 0) {
            result.error = "DELETE chapter failed";
            return result;
        }
        auto upd = topic_dao.update(topic);
        if (upd.is_err()) {
            result.error = upd.error().message;
            return result;
        }
        result.updated = true;
    }

    // 5. 写章节，建立 chapter_no -> id 映射
    db::ChapterDao chapter_dao(db_);
    std::unordered_map<int, int64_t> chapter_no_to_id;

    for (const auto& pc : doc.chapters) {
        db::Chapter c;
        c.topic_id = topic_id;
        c.name = pc.name;
        c.chapter_no = pc.chapter_no;
        if (pc.level == 3 && pc.parent_chapter_no != 0) {
            auto it = chapter_no_to_id.find(pc.parent_chapter_no);
            if (it != chapter_no_to_id.end()) c.parent_id = it->second;
        }
        auto cr = chapter_dao.insert(c);
        if (cr.is_err()) {
            result.error = cr.error().message;
            return result;
        }
        if (pc.level == 2) {
            chapter_no_to_id[pc.chapter_no] = cr.value();
        }
        result.chapters_added++;
    }

    // 6a. 一次性 backfill：把本 topic 下 stable_key=NULL 的旧卡（v3→v4 升级遗留）
    //     按当前 question 文本计算并填入；之后 upsert 才能匹配上、保留历史
    {
        auto sel = db_.prepare(
            "SELECT id, question FROM card "
            "WHERE topic_id = ? AND (stable_key IS NULL OR stable_key = '')");
        if (!sel) {
            result.error = "prepare backfill select failed";
            return result;
        }
        sel.bind(1, topic_id);
        std::vector<std::pair<int64_t, std::string>> rows;
        while (sel.step()) {
            rows.emplace_back(sel.column_int64(0), sel.column_text(1));
        }
        if (!rows.empty()) {
            auto upd = db_.prepare("UPDATE card SET stable_key = ? WHERE id = ?");
            if (!upd) {
                result.error = "prepare backfill update failed";
                return result;
            }
            for (const auto& [id, q] : rows) {
                upd.reset();
                upd.bind(1, compute_stable_key(doc.topic_name, q));
                upd.bind(2, id);
                if (upd.execute() < 0) {
                    result.error = "backfill update failed";
                    return result;
                }
            }
            spdlog::info("backfilled stable_key for {} legacy cards in topic={}",
                rows.size(), doc.topic_name);
        }
    }

    // 6b. 卡片：upsert by stable_key（保留 review 历史），最后清掉本次没出现的
    db::CardDao card_dao(db_);
    std::vector<std::string> seen_keys;
    seen_keys.reserve(doc.cards.size());

    // 检测同一 topic 内题面冲突（不同卡片归一化后一样）
    std::unordered_set<std::string> in_doc_keys;

    for (const auto& pc : doc.cards) {
        db::Card c;
        c.topic_id = topic_id;
        if (auto it = chapter_no_to_id.find(pc.chapter_no);
            it != chapter_no_to_id.end()) {
            c.chapter_id = it->second;
        }
        c.question = pc.question;
        c.answer = pc.answer;
        c.source_line = pc.source_line;
        c.card_type = pc.card_type;
        c.created_at = ts;
        c.updated_at = ts;
        c.stable_key = compute_stable_key(doc.topic_name, pc.question);

        // 同一文档内重复的题面：只保留第一条
        if (!in_doc_keys.insert(c.stable_key).second) {
            spdlog::warn("duplicate question in same doc, skipping: '{}'",
                pc.question.substr(0, 40));
            continue;
        }

        auto cr = card_dao.upsert_by_stable_key(c);
        if (cr.is_err()) {
            result.error = cr.error().message;
            return result;
        }
        seen_keys.push_back(c.stable_key);
        result.cards_added++;
    }

    // 7. 删除本次没出现的卡（stale 卡）—— FK ON DELETE CASCADE 会清 review 历史
    if (!is_new) {
        auto del_r = card_dao.delete_topic_cards_not_in(topic_id, seen_keys);
        if (del_r.is_err()) {
            result.error = del_r.error().message;
            return result;
        }
        if (del_r.value() > 0) {
            spdlog::info("re-import {}: pruned {} stale cards (history lost)",
                doc.topic_name, del_r.value());
        }
    }

    auto cm = txn.commit();
    if (cm.is_err()) {
        result.error = cm.error().message;
        return result;
    }

    spdlog::debug("imported {} (topic={}, cards={})",
        result.file_name, doc.topic_name, result.cards_added);
    return result;
}

}  // namespace bagu::service
