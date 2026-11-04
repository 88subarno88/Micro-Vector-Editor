find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(vec_server_tests AccessTest.cpp AccessTokenTest.cpp ConfigTest.cpp CredentialsTest.cpp LoadDrawingTest.cpp PasswordTest.cpp RoomTest.cpp RateWindowTest.cpp TitleTest.cpp RefreshTokenTest.cpp LogTest.cpp ModelsTest.cpp DocRepoTest.cpp OpRepoTest.cpp UserRepoTest.cpp)
target_link_libraries(vec_server_tests PRIVATE vec_server_common GTest::gtest_main)
set_target_properties(vec_server_tests PROPERTIES CXX_STANDARD 20)

gtest_discover_tests(vec_server_tests)
