#pragma once

#include <nlohmann/json.hpp>
#include <optional>

#include "bagu/error.h"
#include "db/models.h"

namespace bagu::http {

using nlohmann::json;

// ===== Error → JSON =====
inline json error_to_json(const Error& e) {
    return {
        {"error", {
            {"code", e.code},
            {"message", e.message},
            {"detail", e.detail},
        }},
    };
}

/// 错误码 → HTTP status
inline int error_to_http_status(int code) {
    if (code == 0) return 200;
    if (code >= 9000) return 500;
    if (code >= 8000) return 400;
    if (code >= 7000) return 500;
    if (code >= 5000) return 502;
    if (code >= 4020 && code <= 4029) return 404;
    if (code >= 4000) return 500;
    if (code >= 3000) return 500;
    if (code >= 2000) return 400;
    return 500;
}

// ===== Topic =====
inline json to_json(const db::Topic& t) {
    return {
        {"id", t.id},
        {"name", t.name},
        {"title", t.title},
        {"file_path", t.file_path},
        {"file_hash", t.file_hash},
        {"imported_at", t.imported_at},
        {"updated_at", t.updated_at},
    };
}

// ===== Chapter =====
inline json to_json(const db::Chapter& c) {
    json j = {
        {"id", c.id},
        {"topic_id", c.topic_id},
        {"name", c.name},
        {"chapter_no", c.chapter_no},
    };
    if (c.parent_id.has_value()) j["parent_id"] = *c.parent_id;
    else j["parent_id"] = nullptr;
    return j;
}

// ===== Card =====
inline json to_json(const db::Card& c) {
    json j = {
        {"id", c.id},
        {"topic_id", c.topic_id},
        {"question", c.question},
        {"answer", c.answer},
        {"difficulty", c.difficulty},
        {"tags", c.tags},
        {"source_line", c.source_line},
        {"card_type", c.card_type},
        {"created_at", c.created_at},
        {"updated_at", c.updated_at},
    };
    if (c.chapter_id.has_value()) j["chapter_id"] = *c.chapter_id;
    else j["chapter_id"] = nullptr;
    return j;
}

}  // namespace bagu::http

// 引用 review_dao 中的类型，定义放命名空间外避免循环依赖
#include "db/review_dao.h"

namespace bagu::http {

inline json to_json(const db::ReviewRow& r) {
    return {
        {"card_id", r.card_id},
        {"last_review", r.last_review},
        {"next_review", r.next_review},
        {"interval_days", r.interval_days},
        {"ease_factor", r.ease_factor},
        {"repetitions", r.repetitions},
        {"review_count", r.review_count},
        {"correct_count", r.correct_count},
        {"suspended", r.suspended},
    };
}

inline json to_json(const db::DueCard& d) {
    return {
        {"card_id", d.card_id},
        {"topic_id", d.topic_id},
        {"topic_name", d.topic_name},
        {"question", d.question},
        {"answer", d.answer},
        {"card_type", d.card_type},
        {"is_new", d.is_new},
        {"review", to_json(d.state)},
    };
}

}  // namespace bagu::http
