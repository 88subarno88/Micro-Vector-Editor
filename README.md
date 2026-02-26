# Micro-Vector Editor (SVG)

**Course:** COP 290  
**Assignment Type:** Programming Assignment 1 

## Overview
This project is a small vector graphics editor, inspired by tools like Inkscape. Built entirely in C++, it features a graphical user interface (GUI) developed using the Qt community version. The application is designed to read a restricted subset of SVG files, display the vector diagrams on a drawing canvas, allow user modifications, and write the updated diagram back to an SVG output file. 

To ensure clean design and correctness, this editor utilizes a custom-built XML/SVG parser rather than relying on third-party parsing libraries.

---

## Features

### Supported Shapes
The editor allows users to create and modify the following vector graphic elements:
* Rectangle 
* Rounded rectangle 
* Circle 
* Line 
* Hexagon 
* Freehand sketch
* Text 

### Graphical User Interface (GUI)
The UI consists of a menu bar, a toolbar, and a drawing canvas. Any modifications made through the editor are immediately reflected on the canvas and preserved when saving.
* **Shape Properties:** Users can modify the colors for both the boundary (stroke) and the interior (fill).
* **Transformations:** Users can create, move, and resize shapes on the canvas.
* **File Operations:** The menu bar supports New, Open SVG file, Save, Save As, and Close.
* **Edit Operations:** Includes Cut, copy, and paste functionalities.
* **History:** Fully supports Undo and Redo actions utilizing custom stacks.

---

## Software Architecture & Design

### 1. System Overview
The Micro-Vector Editor is designed using a modular architecture that separates the mathematical data models (Core), the user interface (UI), the file I/O operations (Parser), and the action history (Commands). This separation of concerns ensures that the application is robust, scalable, and easy to test. 

### 2. Core Module (The Data Model)
The `core` directory contains the purely mathematical and logical representation of the vector graphics. It is completely decoupled from Qt's windowing system.
* **`GraphicsObject` (Abstract Base Class):** Every shape inherits from this base class. It defines the common properties shared by all vector graphics, such as bounding box coordinates (`x`, `y`, `width`, `height`), `stroke` color, `fill` color, and `stroke-width`. 
* **Polymorphism:** `GraphicsObject` enforces pure virtual functions that every derived shape must implement:
  * `draw(QPainter* painter)`: How the shape renders itself.
  * `toSVG()`: How the shape serializes itself into an XML string.
  * `contains(double x, double y)`: Hit-testing logic to check if a mouse click landed inside the shape.
  * `clone()`: Returns a deep copy of the shape (used heavily for Copy/Paste operations).
* **`Diagram`:** Acts as the centralized state of the canvas. It holds a `std::vector<std::unique_ptr<GraphicsObject>>` representing all active shapes on the screen.

### 3. Command Pattern (Undo / Redo System)
To fulfill the requirement for a robust Undo/Redo system, the application implements the **Command Design Pattern**.
* **`Command` (Interface):** Defines virtual `execute()` and `undo()` methods.
* **Concrete Commands:** Every user action that modifies the canvas state is encapsulated in a specific command object (e.g., `AddCommand`, `MoveCommand`, `DeleteCommand`, `ChangeColorCommand`, `ResizeCommand`). 
* **`CommandStack`:** Manages two internal stacks (one for Undo, one for Redo). This prevents the UI from modifying shapes directly, completely decoupling user inputs from the data model and making history states trivial to manage.

### 4. UI System (View & Controllers)
The graphical user interface is built using **Qt6**.
* **`MainWindow`:** Sets up the application frame, docks the `MenuBar` and `Toolbar`, and sets the central widget.
* **`Canvas` (The Controller):** The most complex UI component. It catches Qt events (mouse clicks, drags, key presses, context menus) and translates them into Core modifications or Command executions.
* ***File Split Architecture:*** To adhere to assignment constraints, the implementation of `Canvas.cpp` was modularized into specific behavioral files (`canvas_Core.cpp`, `canvas_Mouse.cpp`, `canvas_Menu.cpp`, `canvas_Key.cpp`) without breaking the class definition.

### 5. Custom SVG Parser
No third-party XML or SVG parsing libraries were used. The `parser` module implements a custom string-processing engine.
* **Reading (`SVGParser::load_SVG`):** Reads the file line-by-line and uses standard `std::string` operations to locate specific XML tags. 
* **Factory Logic:** Based on the detected tag, the parser converts attributes to `double` via `std::stod`, constructs the appropriate `unique_ptr<GraphicsObject>`, and pushes it directly into the `Diagram`.
* **Writing:** Handled natively by the Core objects. The `Canvas` iterates through the `Diagram`, calling `toSVG()` on each object, and writes the output wrapped in standard `<svg>` tags.

---

## Code Quality & Constraints Adherence
The codebase utilizes structured, readable, and robust design principles:
* **File Length Limits:** The codebase was explicitly refactored to ensure no single `.cpp` or `.h` file exceeds 100 lines. Complex classes have their implementations split across multiple source files.
* **Headers:** Header files (`.h`) strictly contain class definitions, variables, and function signatures. All implementation logic is deferred to the `.cpp` files.
* **Memory Management:** Raw pointers are only used when explicitly required by Qt's object tree (e.g., `QPainter*`, `QWidget*`). All internal data structures (Shapes, Commands) are managed exclusively via `std::unique_ptr` and `std::move` semantics to prevent memory leaks.
* **Style:** The codebase is extensively commented and follows the Google C++ Style Guide conventions for naming, indentation, and standard library usage.

---

## Directory Structure


```text
project
├── build
├── CMakeLists.txt
├── include
│   ├── commands
│   │   ├── AddCommand.h
│   │   ├── ChangecolorCommand.h
│   │   ├── ChangeStrokewidthcommand.h
│   │   ├── Command.h
│   │   ├── CommandStack.h
│   │   ├── CreateCommand.h
│   │   ├── DeleteCommand.h
│   │   ├── ModifyCommand.h
│   │   ├── MoveCommand.h
│   │   └── ResizeCommand.h
│   ├── core
│   │   ├── Circle.h
│   │   ├── Diagram.h
│   │   ├── FreehandSketch.h
│   │   ├── GraphicsObject.h
│   │   ├── Hexagon.h
│   │   ├── Line.h
│   │   ├── Rectangle.h
│   │   ├── RoundedRectangle.h
│   │   └── Text.h
│   ├── parser
│   │   └── SVGParser.h
│   └── ui
│       ├── Canvas.h
│       ├── EditorTypes.h
│       ├── MainWindow.h
│       ├── MenuBar.h
│       └── Toolbar.h
├── README.md
├── src
│   ├── commands
│   │   ├── AddCommand.cpp
│   │   ├── ChangecolorCommand.cpp
│   │   ├── ChangeStrokewidthcommand.cpp
│   │   ├── Command.cpp
│   │   ├── CommandStack.cpp
│   │   ├── CreateCommand.cpp
│   │   ├── DeleteCommand.cpp
│   │   ├── ModifyCommand.cpp
│   │   ├── MoveCommand.cpp
│   │   └── ResizeCommand.cpp
│   ├── core
│   │   ├── Circle.cpp
│   │   ├── Diagram.cpp
│   │   ├── FreehandSketch.cpp
│   │   ├── GraphicsObject.cpp
│   │   ├── Hexagon.cpp
│   │   ├── Line.cpp
│   │   ├── Rectangle.cpp
│   │   ├── RoundedRectangle.cpp
│   │   └── Text.cpp
│   ├── main.cpp
│   ├── parser
│   │   ├── SVGParse_Core.cpp
│   │   └── SVGParse_Parse.cpp
│   └── ui
│       ├── canvas_Core.cpp
│       ├── canvas_Key.cpp
│       ├── canvas_Menu.cpp
│       ├── canvas_Mouse.cpp
│       ├── MainWindow.cpp
│       ├── MenuBar.cpp
│       └── Toolbar.cpp
└── test
    ├── combined_tests.cpp
    └── purpose_of_combined.txt

---

## Prerequisites
* **C++ Compiler:** Modern C++ environment supporting C++ object-oriented features and smart pointers.
* **CMake:** Required for the build system.
* **Qt Framework:** Qt community version is required for rendering the GUI. 

## Compile and Run Instructions
This project utilizes a CMake-based build system. To compile and run the editor, execute the following commands from the root directory (`project/`):

```bash
# 1. go to the build  directory that has already  been created
cd project
mkdir build
cd build

# 2. Configure the project using CMake
cmake ..

# 3. Compile the application
make

# 4. Run the editor
./micro-vector-editor
