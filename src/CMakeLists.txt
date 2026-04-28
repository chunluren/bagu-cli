# 主可执行文件
add_executable(bagu
    main.cpp
)

target_include_directories(bagu
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bagu
    PRIVATE
        CLI11::CLI11
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        tomlplusplus::tomlplusplus
        SQLite::SQLite3
        CURL::libcurl
)

# 后续模块加上：
# add_subdirectory(cli)
# add_subdirectory(db)
# add_subdirectory(parser)
# add_subdirectory(search)
# add_subdirectory(tui)
# add_subdirectory(llm)
# add_subdirectory(core)
# add_subdirectory(util)
