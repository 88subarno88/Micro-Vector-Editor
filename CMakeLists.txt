cmake_minimum_required(VERSION 3.16)
project(MicroVectorEditor VERSION 1.0 LANGUAGES CXX)

# Use C++17 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find Qt6 packages
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

# Enabling automatic Meta-Object Compiler for Qt
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Add include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# List all source files
set(SOURCES
    src/main.cpp
    src/core/GraphicsObject.cpp
    src/core/Circle.cpp   
    src/core/Rectangle.cpp    
    src/core/Line.cpp
    src/core/RoundedRectangle.cpp  
    src/core/Hexagon.cpp  
    src/core/Text.cpp   
    src/core/FreehandSketch.cpp
    src/core/Diagram.cpp
    src/ui/MainWindow.cpp
    src/ui/Toolbar.cpp
    src/ui/Canvas.cpp
    src/ui/MenuBar.cpp
    src/parser/SVGParser.cpp
    src/commands/Command.cpp
    src/commands/CommandStack.cpp
    src/commands/CreateCommand.cpp
    src/commands/DeleteCommand.cpp
    src/commands/MoveCommand.cpp
    src/commands/ModifyCommand.cpp
    src/commands/ResizeCommand.cpp
    src/commands/AddCommand.cpp
    src/commands/ChangecolorCommand.cpp
)

# List all header files 
set(HEADERS
    include/core/GraphicsObject.h
    include/core/Circle.h  
    include/core/Rectangle.h  
    include/core/Line.h
    include/core/RoundedRectangle.h
    include/core/Hexagon.h
    include/core/Text.h
    include/core/FreehandSketch.h
    include/core/Diagram.h
    include/ui/MainWindow.h
    include/ui/Toolbar.h
    include/ui/Canvas.h
    include/ui/MenuBar.h    
    include/ui/EditorTypes.h
    include/parser/SVGParser.h
    include/commands/Command.h
    include/commands/CommandStack.h
    include/commands/CreateCommand.h
    include/commands/DeleteCommand.h
    include/commands/MoveCommand.h
    include/commands/ModifyCommand.h
    include/commands/ResizeCommand.h
    include/commands/AddCommand.h
    include/commands/ChangecolorCommand.h

            
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