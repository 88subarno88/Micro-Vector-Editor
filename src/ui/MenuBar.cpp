#include "ui/MenuBar.h"
#include "ui/MainWindow.h" 
#include <QMenu>
#include <QAction>
#include <QMessageBox> 

void MenuBar::createMenuBar(QMainWindow* mainWindow) {
    QMenuBar* menuBar = mainWindow->menuBar();

    // File Menu
    QMenu* fileMenu = menuBar->addMenu("File");

    // We cast the generic 'mainWindow' to your specific 'MainWindow' to see the custom slot
    MainWindow* mw = dynamic_cast<MainWindow*>(mainWindow);

    // Open Action
    QAction* openAction = fileMenu->addAction("Open");
    if (mw) {
        // Connect "Open" click to the onLoad_Trigger function
        QObject::connect(openAction, &QAction::triggered, mw, &MainWindow::onLoad_Trigger);
    }

    // Save Action
    QAction* saveAction = fileMenu->addAction("Save");
    if (mw) {
        QObject::connect(saveAction, &QAction::triggered, mw, &MainWindow::onSave_Trigger);
    }
    
    // Exit Action
    QAction* exitAction = fileMenu->addAction("Exit");
    QObject::connect(exitAction, &QAction::triggered, mainWindow, &QMainWindow::close);

    // Edit Menu (Placeholder)
    QMenu* editMenu = menuBar->addMenu("Edit");
    // Undo action
    QAction* undoAction = editMenu->addAction("Undo");
    undoAction->setShortcut(QKeySequence::Undo); // ctrl+z
    
    //Redo action
    QAction* redoAction = editMenu->addAction("Redo");
    redoAction->setShortcut(QKeySequence("Ctrl+Y")); // ctrl+y

    // connect with canvas
    if (mw) {
        Canvas* canvas = mw->getCanvas();
        
        if (canvas) {
            QObject::connect(undoAction,&QAction::triggered, canvas, &Canvas::undo);
            QObject::connect(redoAction,&QAction::triggered, canvas, &Canvas::redo);
        }
    }
}