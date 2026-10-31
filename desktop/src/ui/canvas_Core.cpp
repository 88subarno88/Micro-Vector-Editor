#include <QPainter>
#include <fstream>
#include <iostream>

#include "parser/SVGParser.h"
#include "ui/Canvas.h"
#include "ui/QtRenderer.h"

Canvas::Canvas(QWidget* parent) : QWidget(parent), currentMode(ToolType::Select), isDrawing(false) {
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);
}

void Canvas::setTool(ToolType tool) { currentMode = tool; }

void Canvas::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);  // smoothout the edges
    QtRenderer renderer(painter);

    for (const auto& shape : diagram.getShapes()) {
        shape->draw(renderer);
    }
    if (isDrawing && currentShape) {
        currentShape->draw(renderer);
    }
}

void Canvas::saveToFile(const std::string& filename) {
    std::ofstream file(filename);

    // check if file is opened or not
    if (!file.is_open()) {
        std::cerr << "Error: File isnot opening to save  " << std::endl;  // added error
        return;
    }

    file << diagram.toSVG();
}

tl::expected<void, ParseError> Canvas::loadFromFile(const std::string& filename) {
    auto loaded = SVGParser::loadSVG(filename);
    if (!loaded) return tl::unexpected(loaded.error());

    clearDrawing();  // undo history and selection belong to the old diagram
    diagram = std::move(*loaded);
    update();
    return {};
}

void Canvas::run(const vec::Operation& op) {
    if (!commandStack.execute(diagram, op)) std::cerr << "Error: operation rejected" << std::endl;
    update();
}

void Canvas::undo() {
    if (!commandStack.undo(diagram)) std::cerr << "Error: undo failed" << std::endl;
    update();
}

void Canvas::redo() {
    if (!commandStack.redo(diagram)) std::cerr << "Error: redo failed" << std::endl;
    update();
}

void Canvas::clearDrawing() {
    diagram.clear();

    // Clear the undo/redo history
    commandStack.clear();

    // reset
    selectedId.clear();
    isDraggingSelection = false;
    currentShape = nullptr;  // Just in case  something is  being drawm ;safety

    // refresh screen
    update();
}