#ifndef TOOLBAR_H
#define TOOLBAR_H

#include <QToolBar>
#include <QMainWindow>

class Toolbar {
public:
    // Static helper to create the toolbar
    // We pass 'MainWindow*' so we can connect buttons to its slots
    static void createToolbar(QMainWindow* mainWindow);
};

#endif // TOOLBAR_H