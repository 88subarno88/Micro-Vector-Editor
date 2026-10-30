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

    clear_new();  // undo history and selection belong to the old diagram
    diagram = std::move(*loaded);
    update();
    return {};
}

void Canvas::run(const vec::Operation& op) {

    if (!commandStack.execute(diagram, op)) std::cerr << "Error: operation rejected" << std::endl;
    update();
}

void Canvas::undo() {

    // std::cout << "From canvas_Core undo called!" << std::endl;    //Debug
    if (!commandStack.undo(diagram)) std::cerr << "Error: undo failed" << std::endl;
    update(); 
}

void Canvas::redo() {

    // std::cout << "From canvas_Core redo called!" << std::endl;    //Debug
    if (!commandStack.redo(diagram)) std::cerr << "Error: redo failed" << std::endl;
    update();
}

void Canvas::clear_new() {

    // std::cout << "Now canvas_Core gonna delete all shapes via new " << std::endl;    //Debug
    diagram.clearallObjects();

    //Clear the undo/redo history
    commandStack.clear();

    //reset
    selected_Id.clear();
    isDragging_Selection = false;
    current_Shape = nullptr; // Just in case  something is  being drawm ;safety

    //refresh screen
    update();

}