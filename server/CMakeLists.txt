find_package(Drogon CONFIG REQUIRED)

add_library(vec_server_common common/config/Config.cpp)
target_include_directories(vec_server_common PUBLIC common)
find_package(tl-expected CONFIG REQUIRED)
target_link_libraries(vec_server_common PUBLIC tl::expected)
set_target_properties(vec_server_common PROPERTIES CXX_STANDARD 20)
target_compile_options(vec_server_common PRIVATE -Wall -Wextra -pedantic)

add_executable(vec_api api/main.cpp)
target_link_libraries(vec_api PRIVATE vec_core vec_server_common Drogon::Drogon)
set_target_properties(vec_api PROPERTIES
    CXX_STANDARD 20
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
target_compile_options(vec_api PRIVATE -Wall -Wextra -pedantic)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
