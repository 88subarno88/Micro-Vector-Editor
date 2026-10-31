#include "ui/Toolbar.h"

#include <QAction>

#include "ui/MainWindow.h"

void Toolbar::createToolbar(QMainWindow* mainWindow) {
    // cast generic QMainWindow to our specific MainWindow to access slots
    MainWindow* mw = dynamic_cast<MainWindow*>(mainWindow);
    if (!mw) return;

    QToolBar* toolbar = mw->addToolBar("Main Toolbar");
    toolbar->setMovable(false);

    // create actions for toolbar buttons
    QAction* selectAction = toolbar->addAction("Select");
    QAction* circleAction = toolbar->addAction("Circle");
    QAction* rectangleAction = toolbar->addAction("Rectangle");
    QAction* roundedRectAction = toolbar->addAction("Rounded Rectangle");
    QAction* hexagonAction = toolbar->addAction("Hexagon");
    QAction* lineAction = toolbar->addAction("Line");
    QAction* txtAction = toolbar->addAction("Text");
    QAction* freehandAction = toolbar->addAction("Freehand Sketch");

    // connect actions to MainWindow slots
    QObject::connect(selectAction, &QAction::triggered, mw, &MainWindow::setModeSelect);
    QObject::connect(circleAction, &QAction::triggered, mw, &MainWindow::setModeCircle);
    QObject::connect(rectangleAction, &QAction::triggered, mw, &MainWindow::setModeRect);
    QObject::connect(roundedRectAction, &QAction::triggered, mw, &MainWindow::setModeRoundedRect);
    QObject::connect(hexagonAction, &QAction::triggered, mw, &MainWindow::setModeHexagon);
    QObject::connect(lineAction, &QAction::triggered, mw, &MainWindow::setModeLine);
    QObject::connect(txtAction, &QAction::triggered, mw, &MainWindow::setModeText);
    QObject::connect(freehandAction, &QAction::triggered, mw, &MainWindow::setModeFreehand);
}