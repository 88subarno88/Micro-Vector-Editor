cmake_minimum_required(VERSION 3.16)
project(MicroVectorEditor VERSION 1.0 LANGUAGES CXX)

# I am using C++17 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Enable automatic Meta-Object Compiler for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/desktop/include ${CMAKE_SOURCE_DIR}/core/include)

# List all source files
set(SOURCES
    desktop/src/main.cpp
    core/src/core/GraphicsObject.cpp
    core/src/core/Circle.cpp   
    core/src/core/Rectangle.cpp    
    core/src/core/Line.cpp
    core/src/core/RoundedRectangle.cpp  
    core/src/core/Hexagon.cpp  
    core/src/core/Text.cpp   
    core/src/core/FreehandSketch.cpp
    core/src/core/Diagram.cpp
    desktop/src/ui/MainWindow.cpp
    desktop/src/ui/Toolbar.cpp
    desktop/src/ui/canvas_Core.cpp
    desktop/src/ui/canvas_Key.cpp
    desktop/src/ui/canvas_Menu.cpp
    desktop/src/ui/canvas_Mouse.cpp
    desktop/src/ui/MenuBar.cpp
    core/src/parser/SVGParser_Core.cpp
    core/src/parser/SVGParser_Parse.cpp
    core/src/commands/Command.cpp
    core/src/commands/CommandStack.cpp
    core/src/commands/CreateCommand.cpp
    core/src/commands/DeleteCommand.cpp
    core/src/commands/MoveCommand.cpp
    core/src/commands/ModifyCommand.cpp
    core/src/commands/ResizeCommand.cpp
    core/src/commands/AddCommand.cpp
    core/src/commands/ChangecolorCommand.cpp
    core/src/commands/ChangeStrokewidthcommand.cpp
)

# List all header files 
set(HEADERS
    core/include/core/GraphicsObject.h
    core/include/core/Circle.h  
    core/include/core/Rectangle.h  
    core/include/core/Line.h
    core/include/core/RoundedRectangle.h
    core/include/core/Hexagon.h
    core/include/core/Text.h
    core/include/core/FreehandSketch.h
    core/include/core/Diagram.h
    desktop/include/ui/MainWindow.h
    desktop/include/ui/Toolbar.h
    desktop/include/ui/Canvas.h
    desktop/include/ui/MenuBar.h    
    desktop/include/ui/EditorTypes.h
    core/include/parser/SVGParser.h
    core/include/commands/Command.h
    core/include/commands/CommandStack.h
    core/include/commands/CreateCommand.h
    core/include/commands/DeleteCommand.h
    core/include/commands/MoveCommand.h
    core/include/commands/ModifyCommand.h
    core/include/commands/ResizeCommand.h
    core/include/commands/AddCommand.h
    core/include/commands/ChangecolorCommand.h
    core/include/commands/ChangeStrokewidthcommand.h

            
)

# Create the executable
add_executable(micro-vector-editor ${SOURCES} ${HEADERS})

# Link Qt libraries
target_link_libraries(micro-vector-editor
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Compiler warnings 
if(MSVC)
    target_compile_options(micro-vector-editor PRIVATE /W4)
else()
    target_compile_options(micro-vector-editor PRIVATE -Wall -Wextra -pedantic)
endif()