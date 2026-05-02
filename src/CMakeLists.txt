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
        tui/review_screen.cpp
        http/http_server.cpp
        cli/import_cmd.cpp
        cli/list_cmd.cpp
        cli/show_cmd.cpp
        cli/search_cmd.cpp
        cli/review_cmd.cpp
        cli/stats_cmd.cpp
        cli/serve_cmd.cpp
    )
endif()

if(SQLite3_FOUND AND CURL_FOUND)
    list(APPEND BAGU_LIB_SOURCES
        service/interview_service.cpp
        cli/interview_cmd.cpp
    )
endif()

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
