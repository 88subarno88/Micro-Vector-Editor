#include <iostream>
#include <fstream>
#include "ui/Canvas.h"
#include "parser/SVGParser.h"
#include "ui/QtRenderer.h"
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
    QtRenderer renderer(painter);

    for (const auto& shape : diagram.getShapes()) {
        shape->draw(renderer);
    }
    if (isDrawing && current_Shape) {
        current_Shape->draw(renderer);
    }

}

void Canvas::saveToFile(const std::string& filename) {

    std::ofstream file(filename);
    
    //check if file is opened or not
    if (!file.is_open()) {
        std::cerr << "Error: File isnot opening to save  " << std::endl;   //added error
        return;
    }

    file << diagram.toSVG();
}

tl::expected<void, ParseError> Canvas::loadFromFile(const std::string& filename) {

    auto loaded = SVGParser::load_SVG(filename);
    if (!loaded) return tl::unexpected(loaded.error());

    clear_new();  // undo history and selection point into the old diagram
    diagram = std::move(*loaded);
    update();
    return {};
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