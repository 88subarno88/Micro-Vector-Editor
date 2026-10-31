#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QMainWindow>
#include <QMenu>
#include <QToolBar>

#include "ui/Canvas.h"
#include "ui/EditorTypes.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

    // getter for the canvas
    Canvas* getCanvas() const { return canvas; }

    // Slots that the Toolbar buttons will call
public slots:
    void setModeSelect();
    void setModeLine();
    void setModeRect();
    void setModeRoundedRect();
    void setModeCircle();
    void setModeHexagon();
    void setModeText();
    void setModeFreehand();
    void onSaveTrigger();
    void onLoadTrigger();
    void onNew();
    void onSaveAs();

private:
    // Visual Components
    Canvas* canvas;     // the custom drawing widget
    QToolBar* toolbar;  // pointer to the toolbar
    QString currentFilename;
};

#endif  // MAINWINDOW_H