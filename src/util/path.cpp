#include "util/path.h"

#include <cstdlib>

namespace bagu::util {

namespace fs = std::filesystem;

fs::path expand_home(std::string_view path) {
    if (path.empty()) return {};
    if (path[0] != '~') return fs::path(path);

    const char* home = std::getenv("HOME");
    if (!home) return fs::path(path);

    if (path.size() == 1) return fs::path(home);
    if (path[1] != '/') return fs::path(path);

    return fs::path(home) / fs::path(path.substr(2));
}

fs::path default_data_dir() {
    if (const char* env = std::getenv("BAGU_HOME"); env && *env) {
        return expand_home(env);
    }
    return expand_home("~/.bagu");
}

fs::path default_db_path() {
    return default_data_dir() / "bagu.db";
}

fs::path default_config_path() {
    return default_data_dir() / "config.toml";
}

fs::path default_log_dir() {
    return default_data_dir() / "logs";
}

}  // namespace bagu::util
