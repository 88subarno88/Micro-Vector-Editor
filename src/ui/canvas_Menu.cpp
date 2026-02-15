#include "ui/Canvas.h"
#include <QMenu>
#include <QContextMenuEvent>
#include <QInputDialog>
#include <QColorDialog>
#include "commands/DeleteCommand.h"
#include "commands/ResizeCommand.h"
#include "commands/ChangecolorCommand.h"
#include "commands/ChangeStrokewidthcommand.h"

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
    selected_Object = clickedShape;
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
            std::string colorStr = color.name().toStdString();
            auto cmd =std::make_unique<ChangeColorCommand>(selected_Object, colorStr, true);
            commandStack.execute(std::move(cmd));
            update();
        }
    }
    // set stroke color
    else if (selectedItem == setStrokeAction) {
        QColor color = QColorDialog::getColor(Qt::black, this, "Select Stroke Color");
        if (color.isValid()) {
            std::string colorStr =color.name().toStdString();
            auto cmd = std::make_unique<ChangeColorCommand>(selected_Object, colorStr, false);
            commandStack.execute(std::move(cmd));
            update();
        }
    }
    //resize
    else if (selectedItem == resizeAction) {
        bool ok;
        double percentage= QInputDialog::getDouble(this, "Resize", 
                            "Enter Scale % (e.g., 200 = Double):", 
                            100.0, 1.0, 1000.0, 1, &ok);
        if (ok) {
            double factor = percentage / 100.0;
            auto cmd= std::make_unique<ResizeCommand>(selected_Object, factor);
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
    //setwidth
    else if (selectedItem == setWidthAction) {
        bool ok;
        double currentWidth= selected_Object->getStrokeWidth();
        double newWidth = QInputDialog::getDouble(
            this, "Stroke Width", 
            "Enter thickness (e.g., 5.0):", 
            currentWidth, 0.0, 100.0, 1, &ok
        );

        if (ok && newWidth != currentWidth) {
            auto cmd= std::make_unique<ChangeStrokeWidthCommand>(selected_Object, newWidth);
            commandStack.execute(std::move(cmd));
            update(); 
        }
    }
}