cmake_minimum_required(VERSION 3.16)
project(MicroVectorEditor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_DESKTOP "Build the Qt desktop app" ON)
option(BUILD_CLI "Build the vec command-line tool" ON)
option(BUILD_SERVER "Build the Drogon server (needs the vcpkg \"server\" feature)" OFF)
option(BUILD_TESTS "Build the unit tests" ON)
option(BUILD_FUZZERS "Build the libFuzzer targets (needs clang)" OFF)

if(BUILD_TESTS)
    enable_testing()
endif()

add_subdirectory(core)

if(BUILD_CLI)
    add_subdirectory(cli)
endif()

if(BUILD_SERVER)
    add_subdirectory(server)
endif()

if(BUILD_DESKTOP)
    add_subdirectory(desktop)
endif()
