#include <QInputDialog>
#include <QMouseEvent>
#include <algorithm>
#include <cmath>

#include "core/Circle.h"
#include "core/FreehandSketch.h"
#include "core/Hexagon.h"
#include "core/Line.h"
#include "core/Rectangle.h"
#include "core/RoundedRectangle.h"
#include "core/Text.h"
#include "ui/Canvas.h"

void Canvas::mousePressEvent(QMouseEvent* event) {
    // is in select mode
    if (currentMode == ToolType::Select) {
        selectedId.clear();
        isDraggingSelection = false;

        // auto& shapes to modify the actual shapes in the list
        auto& shapes = diagram.getShapes();
        for (auto curr = shapes.rbegin(); curr != shapes.rend(); ++curr) {
            double mouseX = event->position().x();
            double mouseY = event->position().y();

            // Is the click done inside?
            if ((*curr)->contains(mouseX, mouseY)) {
                selectedId = (*curr)->getId();  // grab the shape
                isDraggingSelection = true;     // dragging with selection
                lastMousePos = event->position();
                dragStartPos = event->position();
                update();
                return;
            }
        }
    }
    // unless in selection mode draw the shape
    else {
        isDrawing = true;
        startPoint = event->position();
        currentPoint = event->position();

        if (currentMode == ToolType::FreehandSketch) {
            auto sketch = std::make_unique<FreehandSketch>();
            sketch->addPoint(startPoint.x(), startPoint.y());
            currentShape = std::move(sketch);
        }
    }
}

void Canvas::mouseMoveEvent(QMouseEvent* event) {
    // handle Selection Dragging
    if (currentMode == ToolType::Select) {
        GraphicsObject* selected = selectedShape();
        if (isDraggingSelection && selected) {
            double dx = event->position().x() - lastMousePos.x();
            double dy = event->position().y() - lastMousePos.y();
            selected->move(dx, dy);
            lastMousePos = event->position();
            update();
        }
        return;
    }

    // handle Drawing Preview
    else if (isDrawing) {
        currentPoint = event->position();

        if (currentMode == ToolType::FreehandSketch && currentShape) {
            auto* sketch = dynamic_cast<FreehandSketch*>(currentShape.get());
            if (sketch) sketch->addPoint(currentPoint.x(), currentPoint.y());
        } else if (currentMode != ToolType::Text) {
            double x1 = startPoint.x();
            double y1 = startPoint.y();
            double x2 = currentPoint.x();
            double y2 = currentPoint.y();
            double width = std::abs(x2 - x1);
            double height = std::abs(y2 - y1);
            double topLeftX = std::min(x1, x2);
            double topLeftY = std::min(y1, y2);

            switch (currentMode) {
                case ToolType::Line:
                    currentShape = std::make_unique<Line>(x1, y1, x2, y2);
                    break;
                case ToolType::Rectangle:
                    currentShape = std::make_unique<Rectangle>(topLeftX, topLeftY, width, height);
                    break;
                case ToolType::RoundedRectangle:
                    currentShape =
                        std::make_unique<RoundedRectangle>(topLeftX, topLeftY, width, height, 20.0);
                    break;
                case ToolType::Circle: {
                    double radius = std::max(width, height) / 2.0;
                    currentShape = std::make_unique<Circle>(topLeftX + radius, topLeftY + radius, radius);
                    break;
                }
                case ToolType::Hexagon: {
                    double radius = std::min(width, height) / 2.0;
                    currentShape =
                        std::make_unique<Hexagon>(topLeftX + width / 2.0, topLeftY + height / 2.0, radius);
                    break;
                }
                default:
                    break;
            }
        }
        update();
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent* event) {
    // handle selection mode
    if (currentMode == ToolType::Select) {
        GraphicsObject* selected = selectedShape();
        if (isDraggingSelection && selected) {
            double total_dx = event->position().x() - dragStartPos.x();
            double total_dy = event->position().y() - dragStartPos.y();

            if (std::abs(total_dx) > 0.01 || std::abs(total_dy) > 0.01) {
                selected->move(-total_dx, -total_dy);
                run(vec::Move{selectedId, total_dx, total_dy});
            }
        }
        isDraggingSelection = false;
        update();
        return;
    }

    // stop drawing
    isDrawing = false;
    currentPoint = event->position();
    std::unique_ptr<GraphicsObject> newShape = nullptr;

    // make  the shape finalized
    if (currentMode == ToolType::Text) {
        bool ok;
        QString text =
            QInputDialog::getText(this, "Add Text", "Enter text:", QLineEdit::Normal, "My Text", &ok);
        if (ok && !text.isEmpty()) {
            newShape =
                std::make_unique<Text>(event->position().x(), event->position().y(), text.toStdString());
        }
    } else {
        if (currentShape) newShape = std::move(currentShape);
    }

    // add diagram to the history
    if (newShape) {
        run(vec::AddShape{newShape->getId(), vec::toShapeData(*newShape), std::nullopt});
    }

    currentShape = nullptr;
    update();
}