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
)

# 可选依赖
if(SQLite3_FOUND)
    target_link_libraries(bagu PRIVATE SQLite::SQLite3)
    target_compile_definitions(bagu PRIVATE BAGU_HAVE_SQLITE3)
endif()

if(CURL_FOUND)
    target_link_libraries(bagu PRIVATE CURL::libcurl)
    target_compile_definitions(bagu PRIVATE BAGU_HAVE_CURL)
endif()

# 后续模块加上：
# add_subdirectory(cli)
# add_subdirectory(db)
# add_subdirectory(parser)
# add_subdirectory(search)
# add_subdirectory(tui)
# add_subdirectory(llm)
# add_subdirectory(core)
# add_subdirectory(util)
