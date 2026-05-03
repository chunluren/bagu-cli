add_executable(bagu_tests
    unit/util/path_test.cpp
    unit/result_test.cpp
    unit/parser/markdown_parser_test.cpp
    unit/core/sm2_test.cpp
    unit/llm/mock_client_test.cpp
)

if(SQLite3_FOUND)
    target_sources(bagu_tests PRIVATE
        unit/db/database_test.cpp
        unit/db/statement_test.cpp
        unit/db/migrations_test.cpp
        unit/db/topic_dao_test.cpp
        unit/db/card_dao_test.cpp
        unit/db/interview_dao_test.cpp
        unit/service/review_service_test.cpp
        unit/service/import_service_test.cpp
        unit/service/stats_service_test.cpp
        unit/service/export_service_test.cpp
        unit/http/http_server_test.cpp
    )
endif()

if(SQLite3_FOUND AND CURL_FOUND)
    target_sources(bagu_tests PRIVATE
        unit/service/interview_service_test.cpp
        unit/http/interview_routes_test.cpp
    )
endif()

if(CURL_FOUND)
    target_sources(bagu_tests PRIVATE
        unit/llm/openai_protocol_test.cpp
        unit/llm/config_loader_test.cpp
    )
endif()

target_link_libraries(bagu_tests
    PRIVATE
        bagu_lib
        GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(bagu_tests
    PROPERTIES
        LABELS "unit"
)
