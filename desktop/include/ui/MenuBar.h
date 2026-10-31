#ifndef MENUBAR_H
#define MENUBAR_H

#include <QMainWindow>
#include <QMenuBar>

class MenuBar {
public:
    // static helper to create the menu bar
    static void createMenuBar(QMainWindow* mainWindow);
};

#endif  // MENUBAR_H