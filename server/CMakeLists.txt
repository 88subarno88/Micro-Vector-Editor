find_package(Drogon CONFIG REQUIRED)

add_library(vec_server_common common/auth/Access.cpp common/auth/AccessToken.cpp common/auth/Credentials.cpp common/auth/Password.cpp common/auth/RefreshToken.cpp common/collab/LoadDrawing.cpp common/collab/Messages.cpp common/collab/OpPipeline.cpp common/collab/Presence.cpp common/collab/RemoteOps.cpp common/collab/Room.cpp common/collab/Snapshots.cpp common/config/Config.cpp common/db/Models.cpp common/db/DocRepo.cpp common/db/ExportRepo.cpp common/db/OpRepo.cpp common/db/RefreshTokenRepo.cpp common/db/UserRepo.cpp common/docs/Title.cpp common/exports/RunExport.cpp common/limits/RateWindow.cpp
    common/logging/Log.cpp common/net/ClientAddress.cpp)
target_include_directories(vec_server_common PUBLIC common)
find_package(tl-expected CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(unofficial-sodium CONFIG REQUIRED)
find_package(jwt-cpp CONFIG REQUIRED)
target_link_libraries(vec_server_common PUBLIC vec_core Drogon::Drogon tl::expected nlohmann_json::nlohmann_json PRIVATE vec_render spdlog::spdlog unofficial-sodium::sodium jwt-cpp::jwt-cpp)
set_target_properties(vec_server_common PROPERTIES CXX_STANDARD 20)
target_compile_options(vec_server_common PRIVATE -Wall -Wextra -pedantic)

# Everything in the API but main(), so that tests can run the real server in-process.
add_library(vec_api_lib api/App.cpp api/ApiResponse.cpp api/AuthFilter.cpp api/AuthRoutes.cpp api/DocRoutes.cpp api/ExportRoutes.cpp
    api/HealthRoutes.cpp api/RateLimitFilter.cpp api/Routes.cpp api/RequestLogging.cpp api/collab/DocSocket.cpp)
target_include_directories(vec_api_lib PUBLIC api)
target_link_libraries(vec_api_lib PUBLIC vec_core vec_server_common Drogon::Drogon)
set_target_properties(vec_api_lib PROPERTIES CXX_STANDARD 20)
target_compile_options(vec_api_lib PRIVATE -Wall -Wextra -pedantic)

add_executable(vec_api api/main.cpp)
target_link_libraries(vec_api PRIVATE vec_api_lib)
set_target_properties(vec_api PROPERTIES
    CXX_STANDARD 20
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
target_compile_options(vec_api PRIVATE -Wall -Wextra -pedantic)

add_executable(vec_worker worker/main.cpp)
target_link_libraries(vec_worker PRIVATE vec_server_common Drogon::Drogon)
set_target_properties(vec_worker PROPERTIES
    CXX_STANDARD 20
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
target_compile_options(vec_worker PRIVATE -Wall -Wextra -pedantic)

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()
