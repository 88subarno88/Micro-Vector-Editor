find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(vec_server_tests ConfigTest.cpp LogTest.cpp)
target_link_libraries(vec_server_tests PRIVATE vec_server_common GTest::gtest_main)
set_target_properties(vec_server_tests PROPERTIES CXX_STANDARD 20)

gtest_discover_tests(vec_server_tests)
