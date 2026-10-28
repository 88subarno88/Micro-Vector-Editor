find_package(Qt6 REQUIRED COMPONENTS Widgets)

add_executable(micro-vector-editor
    src/main.cpp
    src/ui/MainWindow.cpp
    src/ui/Toolbar.cpp
    src/ui/MenuBar.cpp
    src/ui/canvas_Core.cpp
    src/ui/canvas_Key.cpp
    src/ui/canvas_Menu.cpp
    src/ui/canvas_Mouse.cpp
    src/ui/QtRenderer.cpp
    # Headers with Q_OBJECT live outside src/, so list them for AUTOMOC.
    include/ui/MainWindow.h
    include/ui/Toolbar.h
    include/ui/MenuBar.h
    include/ui/Canvas.h
    include/ui/EditorTypes.h
)

set_target_properties(micro-vector-editor PROPERTIES
    AUTOMOC ON
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

target_include_directories(micro-vector-editor PRIVATE include)
target_link_libraries(micro-vector-editor PRIVATE vec_core Qt6::Widgets)

if(MSVC)
    target_compile_options(micro-vector-editor PRIVATE /W4)
else()
    target_compile_options(micro-vector-editor PRIVATE -Wall -Wextra -pedantic)
endif()
