#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include "ui/Canvas.h"
#include "ui/EditorTypes.h"
#include <QMenu>      
#include <QAction>    

class MainWindow : public QMainWindow {
    Q_OBJECT    

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    // getter for the canvas
    Canvas* getCanvas() const { return canvas; }

    // Slots that the Toolbar buttons will call
public slots: 
    void setMode_Select();
    void setMode_Line();
    void setMode_Rect();
    void setMode_Roundedrect();
    void setMode_Circle();
    void setMode_Hexagon();
    void setMode_Text();      
    void setMode_Freehand(); 
    void onSave_Trigger();
    void onLoad_Trigger();
    void onNew();
    void onSave_As();

private:
    // Visual Components
    Canvas* canvas;      // the custom drawing widget
    QToolBar* toolbar;   // pointer to the toolbar
    QString currentFilename;

};

#endif // MAINWINDOW_H