#include "ui/Toolbar.h"
#include "ui/MainWindow.h" 
#include <QAction>

void Toolbar::createToolbar(QMainWindow* mainWindow) {
    // cast generic QMainWindow to our specific MainWindow to access slots
    MainWindow* mw = dynamic_cast<MainWindow*>(mainWindow);
    if (!mw) return;

    QToolBar* toolbar = mw->addToolBar("Main Toolbar");
    toolbar->setMovable(false);

    // create actions for toolbar buttons
    QAction* select_Action = toolbar->addAction("Select");
    QAction* circle_Action = toolbar->addAction("Circle");
    QAction* rectangle_Action = toolbar->addAction("Rectangle");
    QAction* roundedRect_Action = toolbar->addAction("Rounded Rectangle");
    QAction* hexagon_Action = toolbar->addAction("Hexagon");
    QAction* line_Action = toolbar->addAction("Line");
    QAction* txt_Action = toolbar->addAction("Text");   
    QAction* freehand_Action = toolbar->addAction("Freehand Sketch");

    // connect actions to MainWindow slots
    QObject::connect(select_Action, &QAction::triggered, mw, &MainWindow::setMode_Select);
    QObject::connect(circle_Action, &QAction::triggered, mw, &MainWindow::setMode_Circle);
    QObject::connect(rectangle_Action, &QAction::triggered, mw, &MainWindow::setMode_Rect);
    QObject::connect(roundedRect_Action, &QAction::triggered, mw, &MainWindow::setMode_Roundedrect);
    QObject::connect(hexagon_Action, &QAction::triggered, mw, &MainWindow::setMode_Hexagon);
    QObject::connect(line_Action, &QAction::triggered, mw, &MainWindow::setMode_Line);
    QObject::connect(txt_Action, &QAction::triggered, mw, &MainWindow::setMode_Text);
    QObject::connect(freehand_Action, &QAction::triggered, mw, &MainWindow::setMode_Freehand);
}