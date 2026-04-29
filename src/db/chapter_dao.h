#pragma once

#include <vector>

#include "bagu/result.h"
#include "db/database.h"
#include "db/models.h"

namespace bagu::db {

class ChapterDao {
public:
    explicit ChapterDao(Database& db) : db_(db) {}

    Result<int64_t> insert(const Chapter& c);
    Result<std::vector<Chapter>> find_by_topic(int64_t topic_id);
    Result<int64_t> count_by_topic(int64_t topic_id);

private:
    Database& db_;
};

}  // namespace bagu::db
