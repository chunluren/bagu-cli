cmake_minimum_required(VERSION 3.20)
project(bagu-cli
    VERSION 0.1.0
    DESCRIPTION "八股文档智能学习助手 — Interview prep CLI tool"
    LANGUAGES CXX
)

# ===== C++ 标准 =====
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 默认 Release
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 编译选项
add_compile_options(-Wall -Wextra -Wpedantic)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    add_compile_options(-g -O0 -fsanitize=address)
    add_link_options(-fsanitize=address)
endif()

# 输出 compile_commands.json 给 clangd 用
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ===== 选项 =====
option(BAGU_BUILD_TESTS "Build unit tests" ON)
option(BAGU_USE_SYSTEM_DEPS "Use system-installed dependencies if found" OFF)

# ===== 第三方依赖（FetchContent）=====
include(FetchContent)

# CLI11 — 命令行解析
FetchContent_Declare(
    CLI11
    GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
    GIT_TAG v2.4.2
    GIT_SHALLOW TRUE
)

# spdlog — 日志
FetchContent_Declare(
    spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
    GIT_SHALLOW TRUE
)

# nlohmann/json
FetchContent_Declare(
    nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW TRUE
)

# toml++ — TOML 配置文件
FetchContent_Declare(
    tomlplusplus
    GIT_REPOSITORY https://github.com/marzer/tomlplusplus.git
    GIT_TAG v3.4.0
    GIT_SHALLOW TRUE
)

# FTXUI — 终端 UI
FetchContent_Declare(
    ftxui
    GIT_REPOSITORY https://github.com/ArthurSonzogni/FTXUI.git
    GIT_TAG v5.0.0
    GIT_SHALLOW TRUE
)
set(FTXUI_BUILD_EXAMPLES OFF CACHE INTERNAL "")
set(FTXUI_BUILD_DOCS OFF CACHE INTERNAL "")
set(FTXUI_BUILD_TESTS OFF CACHE INTERNAL "")

# cpp-httplib — HTTP server（用于 bagu serve）
FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG v0.18.5
    GIT_SHALLOW TRUE
)
set(HTTPLIB_USE_OPENSSL_IF_AVAILABLE OFF CACHE INTERNAL "")  # v0.3 不做 https
set(HTTPLIB_USE_ZLIB_IF_AVAILABLE OFF CACHE INTERNAL "")
set(HTTPLIB_USE_BROTLI_IF_AVAILABLE OFF CACHE INTERNAL "")

FetchContent_MakeAvailable(CLI11 spdlog nlohmann_json tomlplusplus ftxui cpp-httplib)

# ===== 可选依赖（后续阶段需要时启用）=====
# SQLite3
find_package(SQLite3 QUIET)
if(SQLite3_FOUND)
    message(STATUS "SQLite3 found: ${SQLite3_VERSION}")
else()
    message(WARNING "SQLite3 not found, db module will be disabled")
endif()

# libcurl（用于 LLM API）
find_package(CURL QUIET)
if(CURL_FOUND)
    message(STATUS "libcurl found: ${CURL_VERSION_STRING}")
else()
    message(WARNING "libcurl not found, llm module will be disabled")
endif()

# OpenSSL（用于 SHA256 哈希）
find_package(OpenSSL QUIET)
if(OpenSSL_FOUND)
    message(STATUS "OpenSSL found: ${OPENSSL_VERSION}")
else()
    message(FATAL_ERROR "OpenSSL not found (required for SHA256)")
endif()

# ===== GoogleTest（仅在测试构建时拉取）=====
if(BAGU_BUILD_TESTS)
    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
        GIT_SHALLOW TRUE
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

# ===== 源代码 =====
add_subdirectory(src)

# ===== 测试 =====
if(BAGU_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()

# ===== 安装 =====
install(TARGETS bagu
    RUNTIME DESTINATION bin
)

# ===== 摘要 =====
message(STATUS "")
message(STATUS "========== bagu-cli configuration ==========")
message(STATUS "  Version:       ${PROJECT_VERSION}")
message(STATUS "  C++ Standard:  ${CMAKE_CXX_STANDARD}")
message(STATUS "  Build type:    ${CMAKE_BUILD_TYPE}")
message(STATUS "  Build tests:   ${BAGU_BUILD_TESTS}")
message(STATUS "  Compiler:      ${CMAKE_CXX_COMPILER_ID} ${CMAKE_CXX_COMPILER_VERSION}")
message(STATUS "============================================")
message(STATUS "")
