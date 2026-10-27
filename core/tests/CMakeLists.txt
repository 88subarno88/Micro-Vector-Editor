find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(vec_core_tests
    AttributesTest.cpp
    DiagramTest.cpp
    ParserTest.cpp
)

target_link_libraries(vec_core_tests PRIVATE vec_core GTest::gtest_main)

gtest_discover_tests(vec_core_tests)
