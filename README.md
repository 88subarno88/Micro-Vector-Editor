# Micro-Vector Editor (SVG)

[cite_start]**Course:** COP 290 [cite: 2]
[cite_start]**Assignment Type:** Programming Assignment 1 [cite: 3]

## Overview
[cite_start]This project is a small vector graphics editor, inspired by tools like Inkscape[cite: 7]. [cite_start]Built entirely in C++, it features a graphical user interface (GUI) developed using the Qt community version[cite: 14, 46]. [cite_start]The application is designed to read a restricted subset of SVG files, display the vector diagrams on a drawing canvas, allow user modifications, and write the updated diagram back to an SVG output file[cite: 15, 16, 17, 18]. 

[cite_start]To ensure clean design and correctness, this editor utilizes a custom-built XML/SVG parser rather than relying on third-party parsing libraries[cite: 9, 22].

## Features

### Supported Shapes
The editor allows users to create and modify the following vector graphic elements:
* [cite_start]Rectangle [cite: 58]
* [cite_start]Rounded rectangle [cite: 59]
* [cite_start]Circle [cite: 60]
* [cite_start]Line [cite: 61]
* [cite_start]Hexagon [cite: 62]
* [cite_start]Freehand sketch [cite: 63]
* [cite_start]Text [cite: 64]

### Graphical User Interface (GUI)
[cite_start]The UI consists of a menu bar, a toolbar, and a drawing canvas[cite: 48, 53, 54]. [cite_start]Any modifications made through the editor are immediately reflected on the canvas and preserved when saving[cite: 25].
* [cite_start]**Shape Properties:** Users can modify the colors for both the boundary (stroke) and the interior (fill)[cite: 65].
* [cite_start]**Transformations:** Users can create, move, and resize shapes on the canvas[cite: 25].
* [cite_start]**File Operations:** The menu bar supports New, Open SVG file, Save, Save As, and Close[cite: 67, 68, 69].
* [cite_start]**Edit Operations:** Includes Cut, copy, and paste functionalities[cite: 70].
* [cite_start]**History:** Fully supports Undo and Redo actions utilizing custom stacks[cite: 71].

## Architecture & Code Quality
[cite_start]The codebase emphasizes structured, readable, and robust design[cite: 8]. 
* [cite_start]**Object-Oriented Design:** Developed using sophisticated C++ object-oriented features, including smart pointers and inheritance (e.g., shapes deriving from a base `GraphicsObject` class)[cite: 33, 73, 74].
* [cite_start]**Custom SVG Parser:** A built-in parser reads the XML/SVG snippets, generates corresponding shape nodes, and can serialize the models back into valid SVG formats[cite: 23, 28, 31].
* [cite_start]**Modular Codebase:** The project is split into multiple files, with strict adherence to keeping files under 100 lines of code[cite: 75]. [cite_start]Header files exclusively contain class and function signatures[cite: 76].
* [cite_start]**Style:** The code is extensively commented and strictly follows the Google C++ style guide[cite: 79, 80].

## Prerequisites
* [cite_start]**C++ Compiler:** Modern C++ environment supporting C++ object-oriented features and smart pointers[cite: 73, 74].
* [cite_start]**CMake:** Required for the build system[cite: 77].
* [cite_start]**Qt Framework:** Qt community version is required for rendering the GUI[cite: 45, 46]. 

## Compile and Run Instructions
[cite_start]This project utilizes a CMake-based build system[cite: 77]. To compile and run the editor, execute the following commands from the root directory (`project/`):

```bash
# 1. Create a build directory
mkdir build
cd build

# 2. Configure the project using CMake
cmake ..

# 3. Compile the application
make

# 4. Run the editor
./micro-vector-editor