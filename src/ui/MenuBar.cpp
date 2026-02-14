#include "ui/MenuBar.h"
#include "ui/MainWindow.h" 
#include <QMenu>
#include <QAction>
#include <QMessageBox> 

void MenuBar::createMenuBar(QMainWindow* mainWindow) {
    QMenuBar* menuBar = mainWindow->menuBar(); 
    
                                   
    //File has ---> new,open,save,save as, exit buttons
    //edit has ---> undo, redo buttons

                                                                                         
    // File Menu                                                                               
    QMenu* fileMenu = menuBar->addMenu("File");

    // cast the generic 'mainWindow' to  specific 'MainWindow' to see the custom slot
    MainWindow* mw = dynamic_cast<MainWindow*>(mainWindow);

    // Open Action
    QAction* openAction = fileMenu->addAction("Open");

    if (mw) {
        QObject::connect(openAction, &QAction::triggered, mw, &MainWindow::onLoad_Trigger);
    }

    //New Action
    QAction* newAction = fileMenu->addAction("New");
    newAction->setShortcut(QKeySequence::New); // Ctrl+N
    if (mw) {
        QObject::connect(newAction, &QAction::triggered, mw, &MainWindow::onNew);
    }

    // Save Action
    QAction* saveAction = fileMenu->addAction("Save");
    saveAction->setShortcut(QKeySequence::Save);//ctrl+s
    if (mw) {
        QObject::connect(saveAction, &QAction::triggered, mw, &MainWindow::onSave_Trigger);
    }

    //Save as Action 
    QAction* saveAsAction = fileMenu->addAction("Save As");
    saveAsAction->setShortcut(QKeySequence::SaveAs); // Ctrl+Shift+S
    if (mw) {
        QObject::connect(saveAsAction, &QAction::triggered, mw, &MainWindow::onSave_As);
    }
    
    // Exit Action
    QAction* exitAction = fileMenu->addAction("Exit");
    QObject::connect(exitAction, &QAction::triggered, mainWindow, &QMainWindow::close);



    
    // Edit Menu
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