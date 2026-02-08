#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QToolBar>
#include "ui/Canvas.h"
#include "ui/EditorTypes.h"

class MainWindow : public QMainWindow {
    Q_OBJECT    

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
    // Getter for the canvas (if Toolbar needs to talk to it directly later)
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

private:
    // Visual Components
    Canvas* canvas;      // The custom drawing widget
    QToolBar* toolbar;   // Pointer to the toolbar
};

#endif // MAINWINDOW_H