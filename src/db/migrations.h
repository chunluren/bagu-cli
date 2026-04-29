#pragma once

#include "db/database.h"

namespace bagu::db {

/// 注册全部 schema migrations 到给定 database
void register_all_migrations(Database& db);

}  // namespace bagu::db
