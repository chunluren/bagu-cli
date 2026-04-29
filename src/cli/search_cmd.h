#pragma once

#include <string>

namespace bagu::cli {

/// `bagu search <keyword> [--topic T] [--limit N]`
int run_search(const std::string& keyword,
              const std::string& topic,
              int limit);

}  // namespace bagu::cli
