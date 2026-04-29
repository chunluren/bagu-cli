#pragma once

#include <optional>
#include <string>
#include <vector>

#include "bagu/result.h"
#include "db/database.h"
#include "db/models.h"

namespace bagu::db {

class TopicDao {
public:
    explicit TopicDao(Database& db) : db_(db) {}

    /// 插入新 topic，返回新分配的 id（同时设置 t.id）
    Result<int64_t> insert(const Topic& t);

    /// 更新 topic（按 id）
    Result<void> update(const Topic& t);

    /// 按 name 查找
    Result<std::optional<Topic>> find_by_name(const std::string& name);

    /// 按 id 查找
    Result<std::optional<Topic>> find_by_id(int64_t id);

    /// 列出全部
    Result<std::vector<Topic>> find_all();

    /// 按 id 删除（CASCADE 清理 chapter / card）
    Result<void> remove(int64_t id);

private:
    Database& db_;
};

}  // namespace bagu::db
