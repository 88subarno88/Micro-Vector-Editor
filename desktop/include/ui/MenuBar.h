#ifndef MENUBAR_H
#define MENUBAR_H

#include <QMenuBar>
#include <QMainWindow>

class MenuBar {
public:
    // static helper to create the menu bar
    static void createMenuBar(QMainWindow* mainWindow);
};

#endif // MENUBAR_H