#include "ui/Canvas.h"
#include <QMenu>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QColorDialog>

void Canvas::contextMenuEvent(QContextMenuEvent *event) {
    // 1. Check if we clicked on a shape
    GraphicsObject* clickedShape = nullptr;
    for (auto it = diagram.getShapes().rbegin(); it != diagram.getShapes().rend(); ++it) {
        if ((*it)->contains(event->x(), event->y())) {
            clickedShape =it->get();
            break;
        }
    }

    if (!clickedShape) {
        return;
    }

    // select the shape
    selected_Id = clickedShape->getId();
    update(); 

    // creating the Menu
    QMenu menu(this);
    QAction* setFillAction =menu.addAction("Set Fill Color");
    QAction* setStrokeAction =menu.addAction("Set Stroke Color");
    QAction* setWidthAction =menu.addAction("Set Stroke Width");
    QAction* resizeAction =menu.addAction("Resize");
    QAction* deleteAction =menu.addAction("Delete");

    // show Menu and wait to click
    QAction* selectedItem =menu.exec(event->globalPos());

    // set the fill color
    if (selectedItem == setFillAction) {
        QColor color = QColorDialog::getColor(Qt::white, this, "Select Fill Color");
        if (color.isValid()) {
            run(vec::SetStyle{selected_Id, std::nullopt, color.name().toStdString(), std::nullopt});
        }
    }
    // set stroke color
    else if (selectedItem == setStrokeAction) {
        QColor color = QColorDialog::getColor(Qt::black, this, "Select Stroke Color");
        if (color.isValid()) {
            run(vec::SetStyle{selected_Id, color.name().toStdString(), std::nullopt, std::nullopt});
        }
    }
    //resize
    else if (selectedItem == resizeAction) {
        bool ok;
        double percentage= QInputDialog::getDouble(this, "Resize", 
                            "Enter Scale % (e.g., 200 = Double):", 
                            100.0, 1.0, 1000.0, 1, &ok);
        if (ok) {
            run(vec::Resize{selected_Id, percentage / 100.0});
        }
    }
    //delete
    else if (selectedItem == deleteAction) {
        run(vec::DeleteShape{selected_Id});
        selected_Id.clear();
    }
    //setwidth
    else if (selectedItem == setWidthAction) {
        bool ok;
        double currentWidth= clickedShape->getStrokeWidth();
        double newWidth = QInputDialog::getDouble(
            this, "Stroke Width", 
            "Enter thickness (e.g., 5.0):", 
            currentWidth, 0.0, 100.0, 1, &ok
        );

        if (ok && newWidth != currentWidth) {
            run(vec::SetStyle{selected_Id, std::nullopt, std::nullopt, newWidth});
        }
    }
}