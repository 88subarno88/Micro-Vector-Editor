#include <iostream>
#include <cmath>
#include <algorithm>
#include "ui/Canvas.h"
#include <QMouseEvent>
#include <QInputDialog>
#include "core/Circle.h"
#include "core/Rectangle.h"
#include "core/Hexagon.h"
#include "core/Text.h"
#include "core/FreehandSketch.h"
#include "core/Line.h"
#include "core/RoundedRectangle.h"
#include "commands/MoveCommand.h"
#include "commands/AddCommand.h"

void Canvas::mousePressEvent(QMouseEvent *event) {
    // is in select mode 
    if (currentMode == ToolType::Select) {
        selected_Object = nullptr;
        isDragging_Selection = false;
        
        // auto& shapes to modify the actual shapes in the list
        auto& shapes = diagram.getShapes();
        for (auto curr = shapes.rbegin(); curr != shapes.rend(); ++curr) {
            double mouseX = event->position().x();
            double mouseY = event->position().y();

            //Is the click done inside?
            if ((*curr)->contains(mouseX, mouseY)) {
                selected_Object = curr->get();      // grab the shape
                isDragging_Selection = true;        // dragging with selection
                last_Mousepos = event->position(); 
                drag_StartPos = event->position();
                update();
                return;
            }
        }
    } 
    // unless in selection mode draw the shape
    else {
        isDrawing = true;
        start_Point = event->position();
        curr_Point = event->position();

        if (currentMode == ToolType::FreehandSketch) {
            auto sketch = std::make_unique<FreehandSketch>();
            sketch->addPoint(start_Point.x(), start_Point.y());
            current_Shape = std::move(sketch); 
        }
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
    // handle Selection Dragging 
    if (currentMode == ToolType::Select) {
        if (isDragging_Selection && selected_Object) {
            double dx = event->position().x() - last_Mousepos.x();
            double dy = event->position().y() - last_Mousepos.y();
            selected_Object->move(dx, dy);
            last_Mousepos = event->position();
            update();
        }
        return;
    } 
    
    // handle Drawing Preview
    else if (isDrawing) {
        curr_Point = event->position();

        if (currentMode == ToolType::FreehandSketch && current_Shape) {
            auto* sketch = dynamic_cast<FreehandSketch*>(current_Shape.get());
            if (sketch) sketch->addPoint(curr_Point.x(), curr_Point.y());
        }
        else if (currentMode != ToolType::Text) { 
            double x1 = start_Point.x();
            double y1 = start_Point.y();
            double x2 = curr_Point.x();
            double y2 = curr_Point.y();
            double width = std::abs(x2 - x1);
            double height = std::abs(y2 - y1);
            double topLeft_X = std::min(x1, x2); 
            double topLeft_Y = std::min(y1, y2); 

            switch (currentMode) {
                case ToolType::Line: current_Shape = std::make_unique<Line>(x1, y1, x2, y2); break;
                case ToolType::Rectangle: current_Shape = std::make_unique<Rectangle>(topLeft_X, topLeft_Y, width, height); break;
                case ToolType::RoundedRectangle: current_Shape = std::make_unique<RoundedRectangle>(topLeft_X, topLeft_Y, width, height, 20.0); break;
                case ToolType::Circle: {
                    double radius = std::max(width, height) / 2.0;
                    current_Shape = std::make_unique<Circle>(topLeft_X + radius, topLeft_Y + radius, radius);
                    break;
                }
                case ToolType::Hexagon: {
                    double radius = std::min(width, height) / 2.0;
                    current_Shape = std::make_unique<Hexagon>(topLeft_X + width/2.0, topLeft_Y + height/2.0, radius);
                    break;
                }
                default: break;
            }
        }
        update(); 
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
    // handle selection mode
    if (currentMode == ToolType::Select) {
        if (isDragging_Selection && selected_Object) {
            double total_dx = event->position().x() - drag_StartPos.x();
            double total_dy = event->position().y() - drag_StartPos.y();

            if (std::abs(total_dx) > 0.01 || std::abs(total_dy) > 0.01) {
                selected_Object->move(-total_dx, -total_dy);
                auto cmd = std::make_unique<MoveCommand>(selected_Object, total_dx, total_dy);
                commandStack.execute(std::move(cmd));
                // std::cout << "Move saved to Undo stack." << std::endl;                      //Debug
            }
        }
        isDragging_Selection = false; 
        update();                     
        return;                      
    }

    // stop drawing
    isDrawing = false;
    curr_Point = event->position();
    std::unique_ptr<GraphicsObject> newShape = nullptr;

    // make  the shape finalized
    if (currentMode == ToolType::Text) {
        bool ok;
        QString text = QInputDialog::getText(this, "Add Text", "Enter text:", QLineEdit::Normal, "My Text", &ok);
        if (ok && !text.isEmpty()) {
            newShape = std::make_unique<Text>(event->position().x(), event->position().y(), text.toStdString());
        }
    } 
    else {
        if (current_Shape) newShape = std::move(current_Shape);
    }

    // add diagram to the history
    if (newShape) {
        auto cmd = std::make_unique<AddCommand>(&diagram, std::move(newShape));
        commandStack.execute(std::move(cmd));
    }
    
    current_Shape = nullptr; 
    update();
}