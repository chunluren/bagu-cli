#include "cli/init_cmd.h"

#include <spdlog/spdlog.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <system_error>

#include "bagu/error.h"
#include "util/path.h"

namespace bagu::cli {

namespace fs = std::filesystem;

namespace {

constexpr const char* kDefaultConfig = R"([general]
# 数据目录（默认 ~/.bagu，可被环境变量 BAGU_HOME 覆盖）
data_dir = "~/.bagu"

# 八股文档默认路径（bagu import 默认从这里读）
docs_dir = "~/workspaces/bagu-docs"

# 默认主题（list / search 不指定时使用）
default_topic = ""

[review]
# 每天目标复习题数
daily_target = 20
# 每天最多新学题数
new_cards_per_day = 5

[search]
# 搜索结果默认条数
default_limit = 20

[llm]
# LLM 提供商：openai / claude / ollama
provider = "openai"
# API key 环境变量名（不直接存 key）
api_key_env = "OPENAI_API_KEY"
# 模型
model = "gpt-4o-mini"
# 自定义 endpoint（OpenAI 兼容 / Ollama 用）
base_url = ""

[ui]
# 颜色主题：dark / light
theme = "dark"
)";

bool create_dir_if_missing(const fs::path& dir) {
    std::error_code ec;
    if (fs::exists(dir, ec)) {
        return fs::is_directory(dir, ec);
    }
    if (!fs::create_directories(dir, ec)) {
        spdlog::error("创建目录失败: {} ({})", dir.string(), ec.message());
        return false;
    }
    // 数据目录权限设 0700（按安全清单）
    fs::permissions(dir,
        fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec,
        fs::perm_options::replace, ec);
    return true;
}

bool write_default_config(const fs::path& path, bool force) {
    std::error_code ec;
    if (fs::exists(path, ec) && !force) {
        spdlog::info("配置文件已存在，跳过: {}", path.string());
        return true;
    }

    std::ofstream out(path, std::ios::trunc);
    if (!out) {
        spdlog::error("无法写入配置文件: {}", path.string());
        return false;
    }
    out << kDefaultConfig;
    out.close();

    fs::permissions(path,
        fs::perms::owner_read | fs::perms::owner_write,
        fs::perm_options::replace, ec);
    return true;
}

}  // namespace

int run_init(bool force) {
    auto data_dir = util::default_data_dir();
    auto config_path = util::default_config_path();
    auto log_dir = util::default_log_dir();

    std::cout << "Initializing bagu...\n";

    // 1. 创建数据目录
    if (!create_dir_if_missing(data_dir)) {
        std::cerr << "\nError: 创建数据目录失败 (E" << static_cast<int>(E::kDirCreateFailed) << ")\n";
        return to_exit_code(static_cast<int>(E::kDirCreateFailed));
    }
    std::cout << "  ✓ data dir:   " << data_dir.string() << "\n";

    // 2. 创建日志目录
    if (!create_dir_if_missing(log_dir)) {
        std::cerr << "\nError: 创建日志目录失败 (E" << static_cast<int>(E::kDirCreateFailed) << ")\n";
        return to_exit_code(static_cast<int>(E::kDirCreateFailed));
    }
    std::cout << "  ✓ log dir:    " << log_dir.string() << "\n";

    // 3. 写默认配置
    if (!write_default_config(config_path, force)) {
        std::cerr << "\nError: 写配置文件失败 (E" << static_cast<int>(E::kFileWriteFailed) << ")\n";
        return to_exit_code(static_cast<int>(E::kFileWriteFailed));
    }
    std::cout << "  ✓ config:     " << config_path.string() << "\n";

    // 4. 数据库（暂未实现，等 db 模块）
    std::cout << "  ⏭ database:   (skipped, will create on first import)\n";

    std::cout << "\nNext:\n";
    std::cout << "  bagu import <path-to-markdown-docs>\n";
    return 0;
}

}  // namespace bagu::cli
