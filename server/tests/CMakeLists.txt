find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(vec_server_tests AccessTest.cpp AccessTokenTest.cpp ConfigTest.cpp CredentialsTest.cpp LoadDrawingTest.cpp MessagesTest.cpp PasswordTest.cpp PresenceTest.cpp RemoteOpsTest.cpp RunExportTest.cpp RoomTest.cpp StageTimerTest.cpp TokenBucketTest.cpp RateWindowTest.cpp TitleTest.cpp RefreshTokenTest.cpp LogTest.cpp ModelsTest.cpp DocRepoTest.cpp ExportRepoTest.cpp OpPipelineTest.cpp OpRepoTest.cpp UserRepoTest.cpp)
target_link_libraries(vec_server_tests PRIVATE vec_server_common GTest::gtest_main)
set_target_properties(vec_server_tests PROPERTIES CXX_STANDARD 20)

# Database tests skip themselves without TEST_DATABASE_URL; report them as skipped, not passed.
gtest_discover_tests(vec_server_tests PROPERTIES SKIP_REGULAR_EXPRESSION "\\[  SKIPPED \\]")

# The real API in-process, over HTTP.
add_executable(vec_http_tests http/HttpTest.cpp http/HttpServerTest.cpp http/AuthHttpTest.cpp http/DocHttpTest.cpp
    http/PermissionHttpTest.cpp http/DocClient.cpp http/SyncHttpTest.cpp http/PropertyHttpTest.cpp)
target_link_libraries(vec_http_tests PRIVATE vec_api_lib GTest::gtest_main)
target_compile_definitions(vec_http_tests PRIVATE FIXTURE_DIR="${CMAKE_SOURCE_DIR}/core/tests/fixtures")
set_target_properties(vec_http_tests PROPERTIES CXX_STANDARD 20)
gtest_discover_tests(vec_http_tests PROPERTIES SKIP_REGULAR_EXPRESSION "\\[  SKIPPED \\]")
