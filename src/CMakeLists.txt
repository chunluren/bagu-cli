# 核心代码库（让单测也能链接）
set(BAGU_LIB_SOURCES
    util/path.cpp
    util/hash.cpp
    cli/init_cmd.cpp
    cli/config_cmd.cpp
    parser/markdown_parser.cpp
    core/sm2.cpp
)

if(CURL_FOUND)
    list(APPEND BAGU_LIB_SOURCES
        util/http.cpp
        llm/openai_client.cpp
        llm/factory.cpp
        llm/prompts.cpp
        llm/config_loader.cpp
    )
endif()

if(SQLite3_FOUND)
    list(APPEND BAGU_LIB_SOURCES
        db/database.cpp
        db/statement.cpp
        db/migrations.cpp
        db/topic_dao.cpp
        db/chapter_dao.cpp
        db/card_dao.cpp
        db/review_dao.cpp
        db/interview_dao.cpp
        service/import_service.cpp
        service/review_service.cpp
        service/stats_service.cpp
        service/export_service.cpp
        tui/review_screen.cpp
        http/http_server.cpp
        cli/import_cmd.cpp
        cli/list_cmd.cpp
        cli/show_cmd.cpp
        cli/search_cmd.cpp
        cli/review_cmd.cpp
        cli/stats_cmd.cpp
        cli/serve_cmd.cpp
        cli/export_cmd.cpp
    )
endif()

if(SQLite3_FOUND AND CURL_FOUND)
    list(APPEND BAGU_LIB_SOURCES
        service/interview_service.cpp
        cli/interview_cmd.cpp
        http/interview_routes.cpp
    )
endif()

# ===== 嵌入式 Web 资源（v0.3+）=====
# 当 web/dist 存在时自动嵌入；否则用 stub（serve 时返回 404）
set(WEB_DIST_DIR ${CMAKE_SOURCE_DIR}/web/dist)
set(EMBEDDED_ASSETS_FILE ${CMAKE_BINARY_DIR}/embedded_assets.cpp)

if(EXISTS ${WEB_DIST_DIR} AND EXISTS ${WEB_DIST_DIR}/index.html)
    file(GLOB_RECURSE WEB_ASSET_FILES ${WEB_DIST_DIR}/*)
    add_custom_command(
        OUTPUT ${EMBEDDED_ASSETS_FILE}
        COMMAND ${CMAKE_COMMAND}
            -DDIR=${WEB_DIST_DIR}
            -DOUT=${EMBEDDED_ASSETS_FILE}
            -P ${CMAKE_SOURCE_DIR}/scripts/gen_embedded_assets.cmake
        DEPENDS ${WEB_ASSET_FILES} ${CMAKE_SOURCE_DIR}/scripts/gen_embedded_assets.cmake
        COMMENT "Generating embedded web assets"
        VERBATIM
    )
    message(STATUS "Web UI: embedding from ${WEB_DIST_DIR}")
else()
    # Stub：empty asset table
    file(WRITE ${EMBEDDED_ASSETS_FILE}
        "// AUTO-GENERATED stub (web/dist not found, run 'cd web && npm run build')\n"
        "#include \"http/embedded_assets.h\"\n"
        "namespace bagu::http {\n"
        "const EmbeddedAsset* lookup_embedded_asset(std::string_view) { return nullptr; }\n"
        "size_t embedded_asset_count() { return 0; }\n"
        "}\n")
    message(STATUS "Web UI: not built yet (run 'cd web && npm run build')")
endif()

list(APPEND BAGU_LIB_SOURCES ${EMBEDDED_ASSETS_FILE})

add_library(bagu_lib STATIC ${BAGU_LIB_SOURCES})

target_include_directories(bagu_lib
    PUBLIC
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(bagu_lib
    PUBLIC
        spdlog::spdlog
        nlohmann_json::nlohmann_json
        tomlplusplus::tomlplusplus
        OpenSSL::Crypto
        ftxui::screen
        ftxui::dom
        ftxui::component
        httplib::httplib
)

if(SQLite3_FOUND)
    target_link_libraries(bagu_lib PUBLIC SQLite::SQLite3)
    target_compile_definitions(bagu_lib PUBLIC BAGU_HAVE_SQLITE3)
endif()

if(CURL_FOUND)
    target_link_libraries(bagu_lib PUBLIC CURL::libcurl)
    target_compile_definitions(bagu_lib PUBLIC BAGU_HAVE_CURL)
endif()

# 主可执行文件
add_executable(bagu main.cpp)
target_link_libraries(bagu PRIVATE bagu_lib CLI11::CLI11)

# 后续模块加上：
# add_subdirectory(cli)
# add_subdirectory(db)
# add_subdirectory(parser)
# add_subdirectory(search)
# add_subdirectory(tui)
# add_subdirectory(llm)
# add_subdirectory(core)
# add_subdirectory(util)
