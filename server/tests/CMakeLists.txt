find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

add_executable(vec_server_tests AccessTest.cpp AccessTokenTest.cpp ConfigTest.cpp CredentialsTest.cpp LoadDrawingTest.cpp MessagesTest.cpp PasswordTest.cpp PresenceTest.cpp RemoteOpsTest.cpp RunExportTest.cpp RoomTest.cpp StageTimerTest.cpp TokenBucketTest.cpp RateWindowTest.cpp TitleTest.cpp RefreshTokenTest.cpp LogTest.cpp ModelsTest.cpp DocRepoTest.cpp ExportRepoTest.cpp OpPipelineTest.cpp OpRepoTest.cpp UserRepoTest.cpp)
target_link_libraries(vec_server_tests PRIVATE vec_server_common GTest::gtest_main)
set_target_properties(vec_server_tests PROPERTIES CXX_STANDARD 20)

# Database tests skip themselves without TEST_DATABASE_URL; report them as skipped, not passed.
gtest_discover_tests(vec_server_tests PROPERTIES SKIP_REGULAR_EXPRESSION "\\[  SKIPPED \\]")
