#include <iostream>
#include "ui/Canvas.h"
#include <QKeyEvent>

void Canvas::keyPressEvent(QKeyEvent *event) {
    
    // Copy
    if (event->matches(QKeySequence::Copy)) {
        if (const GraphicsObject* selected = selectedShape()) {
            clipboard = selected->clone();
            // std::cout << "Copied " << selected->getType() << " to clipboard via canvas_Key." << std::endl;           //Debug
        }
        return;
    }

    // Paste
    if (event->matches(QKeySequence::Paste)) {
        if (clipboard) {
            auto newShape = clipboard->clone();
            newShape->move(20, 20);                   // donot make the paste obj overlap on the original obj  by 20px
            run(vec::AddShape{newShape->getId(), vec::toShapeData(*newShape), std::nullopt});
            // std::cout << "Pasted shape from clipboard via canvas_Key ." << std::endl;           //Debug
        }
        return;
    }

    // Cut
    if (event->matches(QKeySequence::Cut)) {
        if (const GraphicsObject* selected = selectedShape()) {
            clipboard = selected->clone();
            run(vec::DeleteShape{selected_Id});
            selected_Id.clear();
            // std::cout << "Cut shape to clipboard via canvas_Key ." << std::endl;                   //Debug
        }
        return;
    }

    // Delete / Backspace
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (selectedShape()) {
            run(vec::DeleteShape{selected_Id});
            selected_Id.clear();
            isDragging_Selection = false;
            // std::cout << "Shape Deleted via Keyboard shortcut via canvas_Key." << std::endl;          //Debug
        }
    }
}