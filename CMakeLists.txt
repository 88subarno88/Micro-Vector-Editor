cmake_minimum_required(VERSION 3.16)
project(MicroVectorEditor VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(core)

# Desktop app (moves to desktop/CMakeLists.txt in the next step)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

add_executable(micro-vector-editor
    desktop/src/main.cpp
    desktop/src/ui/MainWindow.cpp
    desktop/src/ui/Toolbar.cpp
    desktop/src/ui/MenuBar.cpp
    desktop/src/ui/canvas_Core.cpp
    desktop/src/ui/canvas_Key.cpp
    desktop/src/ui/canvas_Menu.cpp
    desktop/src/ui/canvas_Mouse.cpp
    # Headers with Q_OBJECT live outside src/, so list them for AUTOMOC.
    desktop/include/ui/MainWindow.h
    desktop/include/ui/Toolbar.h
    desktop/include/ui/MenuBar.h
    desktop/include/ui/Canvas.h
    desktop/include/ui/EditorTypes.h
)

target_include_directories(micro-vector-editor PRIVATE desktop/include)
target_link_libraries(micro-vector-editor PRIVATE vec_core Qt6::Widgets)

if(MSVC)
    target_compile_options(micro-vector-editor PRIVATE /W4)
else()
    target_compile_options(micro-vector-editor PRIVATE -Wall -Wextra -pedantic)
endif()
