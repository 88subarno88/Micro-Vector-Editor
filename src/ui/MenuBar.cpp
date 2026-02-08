#include "ui/MenuBar.h"
#include "ui/MainWindow.h" 
#include <QMenu>
#include <QAction>
#include <QMessageBox> 

void MenuBar::createMenuBar(QMainWindow* mainWindow) {
    QMenuBar* menuBar = mainWindow->menuBar();

    // File Menu
    QMenu* fileMenu = menuBar->addMenu("File");

    // save action
    QAction* saveAction = fileMenu->addAction("Save");
    // We cast the generic 'mainWindow' to your specific 'MainWindow' to see the custom slot
    MainWindow* mw = dynamic_cast<MainWindow*>(mainWindow);
    if (mw) {
        QObject::connect(saveAction, &QAction::triggered, mw, &MainWindow::onSave_Trigger);
    }
    
    //exit  action
    QAction* exitAction = fileMenu->addAction("Exit");
    QObject::connect(exitAction, &QAction::triggered, mainWindow, &QMainWindow::close);

    // Edit Menu (Placeholder)
    QMenu* editMenu = menuBar->addMenu("Edit");
    editMenu->addAction("Undo");
    editMenu->addAction("Redo");
}