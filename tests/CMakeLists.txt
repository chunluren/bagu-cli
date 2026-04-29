add_executable(bagu_tests
    unit/util/path_test.cpp
    unit/result_test.cpp
    unit/parser/markdown_parser_test.cpp
)

if(SQLite3_FOUND)
    target_sources(bagu_tests PRIVATE
        unit/db/database_test.cpp
        unit/db/statement_test.cpp
        unit/db/migrations_test.cpp
        unit/db/topic_dao_test.cpp
        unit/db/card_dao_test.cpp
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
