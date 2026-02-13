#include <iostream>
#include <fstream>
#include <algorithm>
#include <cmath>
#include "ui/Canvas.h"
#include "parser/SVGParser.h"
#include "commands/MoveCommand.h"
#include "commands/DeleteCommand.h"
#include "commands/ResizeCommand.h"
#include "commands/DeleteCommand.h"


// Qts
#include <QMenu>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QPainter>
#include <QApplication>
#include <QKeySequence>
#include <QColorDialog>



// Shape Includes (Moved from MainWindow.cpp)
#include "core/Circle.h"
#include "core/Rectangle.h"
#include "core/Hexagon.h"
#include "core/Text.h"
#include "core/FreehandSketch.h"
#include "core/Line.h"
#include "core/RoundedRectangle.h"
#include "commands/CommandStack.h"
#include "commands/CreateCommand.h"
#include "commands/AddCommand.h"
#include "commands/ChangecolorCommand.h"
#include "core/Diagram.h"

Canvas::Canvas(QWidget *parent) 
    : QWidget(parent), currentMode(ToolType::Select), isDrawing(false) {
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
    setFocusPolicy(Qt::StrongFocus);
}

void Canvas::setTool(ToolType tool) {
    currentMode = tool;
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

void Canvas::mousePressEvent(QMouseEvent *event) {
    // is in select mode 
    if (currentMode == ToolType::Select) {
        
        selected_Object = nullptr;
        isDragging_Selection = false;
        
        // auto& shapes to modify the actual shapes in the list
        auto& shapes =diagram.getShapes();
        for (auto curr =shapes.rbegin(); curr!= shapes.rend(); ++curr) {
            
            //converting to double for math we are doing
            double mouseX =event->position().x();
            double mouseY =event->position().y();

            //Is the click done inside?
            if ((*curr)->contains(mouseX, mouseY)) {
                selected_Object=curr->get(); // grab the shape
                isDragging_Selection = true; // dragging with selection
                last_Mousepos = event->position(); // from where we dragged the shape
                drag_StartPos = event->position();// remember from  where dragging started
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
            // Create the sketch immediately so we can add points to it
            auto sketch = std::make_unique<FreehandSketch>();
            
            // Add the very first point (where you clicked)
            sketch->addPoint(start_Point.x(), start_Point.y());
            
            // Store it in the class variable 'currentShape'
            // (Make sure this matches the name in Canvas.h: currentShape or current_Shape)
            current_Shape= std::move(sketch); 
        }
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
    // 1. Handle Selection Dragging (Keep existing logic)
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
    
    // 2. Handle Drawing Preview
    else if (isDrawing) {
        curr_Point = event->position();

        // CASE A: Freehand Sketch (Append points to existing shape)
        if (currentMode == ToolType::FreehandSketch && current_Shape) {
            auto* sketch = dynamic_cast<FreehandSketch*>(current_Shape.get());
            if (sketch) {
                sketch->addPoint(curr_Point.x(), curr_Point.y());
            }
        }
        // CASE B: Standard Shapes (Re-create shape to show preview)
        else if (currentMode != ToolType::Text) { 
            // We skip Text because it uses a popup dialog
            
            // Calculate geometry based on start vs current point
            double x1 = start_Point.x();
            double y1 = start_Point.y();
            double x2 = curr_Point.x();
            double y2 = curr_Point.y();
            double width = std::abs(x2 - x1);
            double height = std::abs(y2 - y1);
            double topLeft_X = std::min(x1, x2); 
            double topLeft_Y = std::min(y1, y2); 

            // Create the temporary preview shape
            switch (currentMode) {
                case ToolType::Line:
                    current_Shape = std::make_unique<Line>(x1, y1, x2, y2);
                    break;
                case ToolType::Rectangle:
                    current_Shape = std::make_unique<Rectangle>(topLeft_X, topLeft_Y, width, height);
                    break;
                case ToolType::RoundedRectangle:
                    current_Shape = std::make_unique<RoundedRectangle>(topLeft_X, topLeft_Y, width, height, 20.0);
                    break;
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

        update(); // Trigger paintEvent to draw 'current_Shape'
    }
}
void Canvas::mouseReleaseEvent(QMouseEvent *event) {
    // 1. Handle Selection Mode (Keep existing logic)
    if (currentMode == ToolType::Select) {
        if (isDragging_Selection && selected_Object) {
            double total_dx = event->position().x() - drag_StartPos.x();
            double total_dy = event->position().y() - drag_StartPos.y();

            if (std::abs(total_dx) > 0.01 || std::abs(total_dy) > 0.01) {
                selected_Object->move(-total_dx, -total_dy);
                auto cmd = std::make_unique<MoveCommand>(selected_Object, total_dx, total_dy);
                commandStack.execute(std::move(cmd));
                std::cout << "Move saved to Undo stack." << std::endl;
            }
        }
        isDragging_Selection = false; 
        update();                     
        return;                      
    }

    // 2. Stop drawing
    isDrawing = false;
    curr_Point = event->position();
    
    std::unique_ptr<GraphicsObject> newShape = nullptr;

    // 3. Finalize the Shape
    if (currentMode == ToolType::Text) {
        // Text is special: It needs a dialog, so we create it here
        bool ok;
        QString text = QInputDialog::getText(this, "Add Text", "Enter text:", QLineEdit::Normal, "My Text", &ok);
        if (ok && !text.isEmpty()) {
            newShape = std::make_unique<Text>(event->position().x(), event->position().y(), text.toStdString());
        }
    } 
    else {
        // For everything else (Rect, Circle, Line, Freehand):
        // We just TAKE the shape we were previewing in mouseMove
        if (current_Shape) {
            newShape = std::move(current_Shape);
        }
    }

    // 4. Add to Diagram (History)
    if (newShape) {
        auto cmd = std::make_unique<AddCommand>(&diagram, std::move(newShape));
        commandStack.execute(std::move(cmd));
    }
    
    // Cleanup
    current_Shape = nullptr; 
    update();
}
void Canvas::saveToFile(const std::string& filename) {
    // creating filestream
    std::ofstream file(filename);
    
    //check if file is opened or not
    if (!file.is_open()) {
        std::cerr << "Error: File isnot opening for saving " << std::endl;
        return;
    }

    //the standard SVG Header
    file << "<svg xmlns=\"http://www.w3.org/2000/svg\" width=\"800\" height=\"600\" version=\"1.1\">\n";

    // diagram.getShapes() gives us the list of objects
    for (const auto& shape : diagram.getShapes()) {
        file << "  " << shape->toSVG() << "\n";
    }

    // footer
    file << "</svg>";

    // close file
    file.close();
}

void Canvas::loadFromFile(const std::string& filename) {
    //pass the diagram
    SVGParser::load_SVG(filename, diagram);
    update(); //redraw newshapes
}

void Canvas::undo() {
    std::cout << "Canvas::undo called!" << std::endl; // <--- ADD THIS
    commandStack.undo();
    update(); 
}

void Canvas::redo() {
    std::cout << "Canvas::redo called!" << std::endl; // <--- ADD THIS
    commandStack.redo();
    update();
}

void Canvas::contextMenuEvent(QContextMenuEvent *event) {
    // 1. Check if we clicked on a shape
    // (Simple hit test: iterate backwards to find top-most shape)
    GraphicsObject* clickedShape = nullptr;
    for (auto it = diagram.getShapes().rbegin(); it != diagram.getShapes().rend(); ++it) {
        if ((*it)->contains(event->x(), event->y())) {
            clickedShape = it->get();
            break;
        }
    }

    // If no shape clicked, do nothing
    if (!clickedShape) return;

    // 2. Select the shape visually
    selected_Object = clickedShape;
    update(); // Redraw so selection box appears

    // 3. Create the Menu
    QMenu menu(this);
    QAction* setFillAction = menu.addAction("Set Fill Color");
    QAction* setStrokeAction = menu.addAction("Set Stroke Color");
    QAction* resizeAction = menu.addAction("Resize");
    QAction* deleteAction = menu.addAction("Delete");

    // 4. Show Menu and wait for user click
    QAction* selectedItem = menu.exec(event->globalPos());
    // set the fill color
    if (selectedItem == setFillAction) {
        // Open Color Picker
        QColor color = QColorDialog::getColor(Qt::white, this, "Select Fill Color");
        
        if (color.isValid()) {
            // Convert QColor to std::string (e.g., "#FF0000")
            std::string colorStr = color.name().toStdString();

            // Create and Execute Command (True = Fill)
            auto cmd = std::make_unique<ChangeColorCommand>(selected_Object, colorStr, true);
            commandStack.execute(std::move(cmd));
            update();
        }
    }
    // set stroke color
    else if (selectedItem == setStrokeAction) {
        QColor color = QColorDialog::getColor(Qt::black, this, "Select Stroke Color");
        
        if (color.isValid()) {
            std::string colorStr = color.name().toStdString();

            // Create and Execute Command (False = Stroke)
            auto cmd = std::make_unique<ChangeColorCommand>(selected_Object, colorStr, false);
            commandStack.execute(std::move(cmd));
            update();
        }
    }
    //resize
    else if (selectedItem == resizeAction) {
        bool ok;
        double percentage = QInputDialog::getDouble(this, "Resize", 
                            "Enter Scale % (e.g., 200 = Double):", 
                            100.0, 1.0, 1000.0, 1, &ok);
        if (ok) {
            double factor = percentage / 100.0;
            auto cmd = std::make_unique<ResizeCommand>(selected_Object, factor);
            commandStack.execute(std::move(cmd));
             update();
        }
    }
    //delete
    else if (selectedItem == deleteAction) {
        auto cmd = std::make_unique<DeleteCommand>(&diagram, selected_Object);
        commandStack.execute(std::move(cmd));
        selected_Object = nullptr;
        update();
    }
}

void Canvas::keyPressEvent(QKeyEvent *event) {
    
    if (event->matches(QKeySequence::Copy)) {
        if (selected_Object) {
            // Create a deep copy and store it in  'clipboard' variable
            clipboard = selected_Object->clone();
            std::cout << "Copied " << selected_Object->getType() << " to clipboard." << std::endl;
        }
        return;
    }

    if (event->matches(QKeySequence::Paste)) {
        if (clipboard) {
            //Create a NEW copy from the clipboard (so we can paste multiple times)
            auto newShape = clipboard->clone();

            //Move it slightly so it doesn't overlap perfectly (Offset by 20px)
            newShape->move(20, 20);

            // Add to the diagram
            // Note: If you want Undo support, use a PasteCommand here. 
            // For now, let's just add it directly to see it work.
            diagram.addObject(std::move(newShape)); 
            
            //Redraw
            update();
            std::cout << "Pasted shape from clipboard." << std::endl;
        }
        return;
    }

    if (event->matches(QKeySequence::Cut)) {
        if (selected_Object) {
            // A. Copy first
            clipboard = selected_Object->clone();

            // B. Then Delete (Using your existing Undo-able Delete Command)
            auto cmd = std::make_unique<DeleteCommand>(&diagram, selected_Object);
            commandStack.execute(std::move(cmd));
            
            // C. Clear selection and update
            selected_Object = nullptr;
            update();
            std::cout << "Cut shape to clipboard." << std::endl;
        }
        return;
    }
    // see if the user pressed Delete or Backspace
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        
        // delete only if shape is selected
        if (selected_Object) {
            
            // 1. Create the Delete Command
            // We pass the diagram and the currently selected object
            auto cmd = std::make_unique<DeleteCommand>(&diagram, selected_Object);
            
            // 2. Execute it (This removes the shape from the screen)
            commandStack.execute(std::move(cmd));
            
            // 3. Clear selection so we don't try to move a deleted object
            selected_Object = nullptr;
            isDragging_Selection = false;
            
            update(); // Redraw canvas to show it's gone
            std::cout << "Shape Deleted via Keyboard shortcut." << std::endl;
        }
    }
}

void Canvas::clear_new() {
    // delete all the shapes
    diagram.clearallObjects();

    //Clear the undo/redo history
    // (You might need to add a clear() function to your CommandStack class too)
    commandStack.clear_(); 

    // 3. Reset all tools and selections
    selected_Object = nullptr;
    isDragging_Selection = false;
    current_Shape = nullptr; // Just in case we were drawing something

    // 4. Refresh the screen so it looks empty
    update();
}