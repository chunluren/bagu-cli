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
