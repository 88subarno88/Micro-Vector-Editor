#include <iostream>
#include <fstream>
#include <QPainter>
#include <algorithm>
#include <cmath>
#include <QInputDialog> 
#include "ui/Canvas.h"


// Shape Includes (Moved from MainWindow.cpp)
#include "core/Circle.h"
#include "core/Rectangle.h"
#include "core/Hexagon.h"
#include "core/Text.h"
#include "core/FreehandSketch.h"
#include "core/Line.h"
#include "core/RoundedRectangle.h"

Canvas::Canvas(QWidget *parent) 
    : QWidget(parent), currentMode(ToolType::Select), isDrawing(false) {
    // Set background to white (Optional but good for drawing apps)
    setBackgroundRole(QPalette::Base);
    setAutoFillBackground(true);
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
    }
}

void Canvas::mouseMoveEvent(QMouseEvent *event) {
    // is in select mode 
    if (currentMode == ToolType::Select) {
        if (isDragging_Selection && selected_Object) {
            
            // calculate the far the pos moved from last time
            double dx =event->position().x()-last_Mousepos.x();
            double dy =event->position().y()-last_Mousepos.y();

            //move shape by same  amount
            selected_Object->move(dx, dy);

            //change the last pos for next movement
            last_Mousepos = event->position();

            //Redraw
            update();
        }
    } 
    
    //keep things as it is if not selectionmode
    else if (isDrawing) {
        curr_Point = event->position();
        update();
    }
}

void Canvas::mouseReleaseEvent(QMouseEvent *event) {
    if (currentMode == ToolType::Select) {
        isDragging_Selection = false; //stop moving the selected shape
        update();                     //refresh
        return;                      
    }
    isDrawing = false;
    curr_Point = event->position();

    // Calculate dimensions
    double x1 = start_Point.x();
    double y1 = start_Point.y();
    double x2 = curr_Point.x();
    double y2 = curr_Point.y();
    double width = std::abs(x2 - x1);
    double height = std::abs(y2 - y1);
    // Qt Top down approach: origin (0,0) is at top-left
    double topLeft_X = std::min(x1, x2); 
    double topLeft_Y = std::min(y1, y2); 

    // Switch case for different modes (Moved from MainWindow)
    switch (currentMode) {
        case ToolType::Select:
            break;
        case ToolType::Line:
            diagram.addObject(std::make_unique<Line>(x1, y1, x2, y2));
            break;
        case ToolType::Rectangle:
            diagram.addObject(std::make_unique<Rectangle>(topLeft_X, topLeft_Y, width, height));
            break;
        case ToolType::RoundedRectangle:
            diagram.addObject(std::make_unique<RoundedRectangle>(topLeft_X, topLeft_Y, width, height, 15.0));
            break;
        case ToolType::Circle: {
            double radius = std::max(width, height) / 2.0;
            double center_X = topLeft_X + radius; 
            double center_Y = topLeft_Y + radius;
            diagram.addObject(std::make_unique<Circle>(center_X, center_Y, radius));
            break;
        }
        case ToolType::Hexagon: {
            double radius = std::min(width, height) / 2.0;
            double c_x = topLeft_X + (width / 2.0);
            double c_y = topLeft_Y + (height / 2.0);
            diagram.addObject(std::make_unique<Hexagon>(c_x, c_y, radius));
            break;
        }
        case ToolType::Text: {
            // Step 1: Prepare the settings for the pop-up box
            QString title = "Add Text";                   // Title of the window
            QString question = "What do you want to write?"; // Text label inside
            QString defaultText = "My Text";              // Text that appears by default
            bool userClickedOK = false;                   // This will become true if they click "OK"

            // Step 2: Show the dialog box and capture the result
            QString typedText = QInputDialog::getText(
                this,           // Parent window (the Canvas)
                title,          // Window title
                question,       // The question label
                QLineEdit::Normal, // Normal text mode (not a password)
                defaultText,    // The starting text
                &userClickedOK  // Updates our boolean variable above
            );

            // Step 3: Check if valid input was received
            // Only proceed if user clicked "OK" AND the text box wasn't empty
            if (userClickedOK == true && typedText.isEmpty() == false) {
                
                // Convert Qt string to standard C++ string
                std::string finalString = typedText.toStdString();
                
                // Add the text object to the diagram
                diagram.addObject(std::make_unique<Text>(topLeft_X, topLeft_Y, finalString));
            }
            break;   
        }
        case ToolType::FreehandSketch: {
            auto sketch = std::make_unique<FreehandSketch>();
            sketch->addPoint(x1, y1);
            sketch->addPoint(x2, y2); 
            diagram.addObject(std::move(sketch));
            break;      
        }  
    }
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