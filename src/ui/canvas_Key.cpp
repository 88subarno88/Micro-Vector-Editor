#include <iostream>
#include "ui/Canvas.h"
#include <QKeyEvent>
#include "commands/DeleteCommand.h"

void Canvas::keyPressEvent(QKeyEvent *event) {
    
    // Copy
    if (event->matches(QKeySequence::Copy)) {
        if (selected_Object) {
            clipboard = selected_Object->clone();
            // std::cout << "Copied " << selected_Object->getType() << " to clipboard via canvas_Key." << std::endl;           //Debug
        }
        return;
    }

    // Paste
    if (event->matches(QKeySequence::Paste)) {
        if (clipboard) {
            auto newShape = clipboard->clone();
            newShape->move(20, 20);                   // donot make the paste obj overlap on the original obj  by 20px
            diagram.addObject(std::move(newShape)); 
            
            update();
            // std::cout << "Pasted shape from clipboard via canvas_Key ." << std::endl;           //Debug
        }
        return;
    }

    // Cut
    if (event->matches(QKeySequence::Cut)) {
        if (selected_Object) {
            clipboard = selected_Object->clone();
            auto cmd = std::make_unique<DeleteCommand>(&diagram, selected_Object);
            commandStack.execute(std::move(cmd));
            
            selected_Object = nullptr;
            update();
            // std::cout << "Cut shape to clipboard via canvas_Key ." << std::endl;                   //Debug
        }
        return;
    }

    // Delete / Backspace
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (selected_Object) {
            auto cmd = std::make_unique<DeleteCommand>(&diagram, selected_Object);
            commandStack.execute(std::move(cmd));
            
            selected_Object = nullptr;
            isDragging_Selection = false;
            
            update(); 
            // std::cout << "Shape Deleted via Keyboard shortcut via canvas_Key." << std::endl;          //Debug
        }
    }
}