find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(vec_core_tests
    DiagramTest.cpp
    FixtureTest.cpp
    ParserHelpersTest.cpp
    ParserTest.cpp
    RenderTest.cpp
    ShapeIdTest.cpp
)

target_link_libraries(vec_core_tests PRIVATE vec_core GTest::gtest_main)
target_compile_definitions(vec_core_tests PRIVATE FIXTURE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/fixtures")

gtest_discover_tests(vec_core_tests)
