cmake_minimum_required(VERSION 3.16)
project(MicroVectorEditor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(BUILD_DESKTOP "Build the Qt desktop app" ON)

add_subdirectory(core)

if(BUILD_DESKTOP)
    add_subdirectory(desktop)
endif()
