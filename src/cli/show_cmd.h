#pragma once

#include <cstdint>

namespace bagu::cli {

/// `bagu show <id>` — 显示单张卡片完整内容
int run_show(int64_t card_id);

}  // namespace bagu::cli
