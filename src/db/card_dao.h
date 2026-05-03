#pragma once

#include <optional>
#include <vector>

#include "bagu/result.h"
#include "db/database.h"
#include "db/models.h"

namespace bagu::db {

class CardDao {
public:
    explicit CardDao(Database& db) : db_(db) {}

    Result<int64_t> insert(const Card& c);

    /// 批量插入（要求外部已开事务，性能高）
    Result<int64_t> insert_batch(const std::vector<Card>& cards);

    /// 按 stable_key upsert：存在则 UPDATE 题面/答案/章节/source_line/updated_at
    /// 并返回该卡片 id（保留 review/review_history）；不存在则 INSERT。
    /// 要求 c.stable_key 非空。
    Result<int64_t> upsert_by_stable_key(const Card& c);

    /// 删除某 topic 下不在 keep_keys 中的卡片
    /// 返回删除条数。用于重导入时清理那些题面被删掉的卡。
    Result<int64_t> delete_topic_cards_not_in(
        int64_t topic_id, const std::vector<std::string>& keep_keys);

    Result<std::optional<Card>> find_by_id(int64_t id);
    Result<std::vector<Card>> find_by_topic(int64_t topic_id);
    Result<int64_t> count_by_topic(int64_t topic_id);

    /// 全文搜索
    /// @param keyword FTS5 query string
    /// @param topic_id 0 = 不限主题
    /// @param limit 上限
    Result<std::vector<Card>> search(const std::string& keyword,
                                     int64_t topic_id, int limit);

    Result<void> remove_by_topic(int64_t topic_id);

private:
    Database& db_;

    static Card parse_row(Statement& stmt);
};

}  // namespace bagu::db
