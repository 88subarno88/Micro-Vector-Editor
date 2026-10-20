#include <iostream>
#include <fstream>
#include "ui/Canvas.h"
#include "parser/SVGParser.h"
#include <QPainter>

Canvas::Canvas(QWidget *parent) 
    : QWidget(parent), currentMode(ToolType::Select), isDrawing(false) {
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);
}

void Canvas::setTool(ToolType tool) {

    currentMode=tool;
}

void Canvas::paintEvent(QPaintEvent *event) {

    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing); // smoothout the edges

    for (const auto& shape : diagram.getShapes()) {
        shape->draw(&painter);
    }
    if (isDrawing && current_Shape) {
        current_Shape->draw(&painter);
    }

}

void Canvas::saveToFile(const std::string& filename) {

    std::ofstream file(filename);
    
    //check if file is opened or not
    if (!file.is_open()) {
        std::cerr << "Error: File isnot opening to save  " << std::endl;   //added error
        return;
    }

    //standard SVG Header
    file << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"600\" version=\"1.1\">\n";

    //get shapes and convert into svg
    for (const auto& shape : diagram.getShapes()) {
        file << "  " << shape->toSVG() << "\n";
    }

    file << "</svg>";
    file.close();
}

void Canvas::loadFromFile(const std::string& filename) {

    SVGParser::load_SVG(filename, diagram);
    update(); 
}

void Canvas::undo() {

    // std::cout << "From canvas_Core undo called!" << std::endl;    //Debug
    commandStack.undo();
    update(); 
}

void Canvas::redo() {

    // std::cout << "From canvas_Core redo called!" << std::endl;    //Debug
    commandStack.redo();
    update();
}

void Canvas::clear_new() {

    // std::cout << "Now canvas_Core gonna delete all shapes via new " << std::endl;    //Debug
    diagram.clearallObjects();

    //Clear the undo/redo history
    commandStack.clear_(); 

    //reset
    selected_Object = nullptr;
    isDragging_Selection = false;
    current_Shape = nullptr; // Just in case  something is  being drawm ;safety

    //refresh screen
    update();

}