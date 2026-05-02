#pragma once

#include <string>

namespace bagu::cli {

struct ServeOptions {
    std::string bind = "127.0.0.1";
    int port = 8780;
    std::string token;
    bool dev = false;
};

int run_serve(const ServeOptions& opts);

}  // namespace bagu::cli
