#include <QKeyEvent>

#include "ui/Canvas.h"

void Canvas::keyPressEvent(QKeyEvent* event) {
    // Copy
    if (event->matches(QKeySequence::Copy)) {
        if (const GraphicsObject* selected = selectedShape()) {
            clipboard = selected->clone();
            // //Debug
        }
        return;
    }

    // Paste
    if (event->matches(QKeySequence::Paste)) {
        if (clipboard) {
            auto newShape = clipboard->clone();
            newShape->move(20, 20);  // donot make the paste obj overlap on the original obj  by 20px
            run(vec::AddShape{newShape->getId(), vec::toShapeData(*newShape), std::nullopt});
        }
        return;
    }

    // Cut
    if (event->matches(QKeySequence::Cut)) {
        if (const GraphicsObject* selected = selectedShape()) {
            clipboard = selected->clone();
            run(vec::DeleteShape{selectedId});
            selectedId.clear();
        }
        return;
    }

    // Delete / Backspace
    if (event->key() == Qt::Key_Delete || event->key() == Qt::Key_Backspace) {
        if (selectedShape()) {
            run(vec::DeleteShape{selectedId});
            selectedId.clear();
            isDraggingSelection = false;
        }
    }
}