#include "ui/MainWindow.h"
#include "ui/Toolbar.h"
#include "ui/MenuBar.h"
#include "ui/Canvas.h"
//Qts
#include <QFileDialog> 
#include <QMessageBox> 

// Constructor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent) {
    
    // set window title and size 
    resize(900, 600);                      //Default
    setWindowTitle("Micro-SVG Editor");    //Default 
     
    //create the Canvas i.e. the drawing area
    canvas = new Canvas(this);
    setCentralWidget(canvas);

    //create toolbar and menu using the helpers
    Toolbar::createToolbar(this);
    MenuBar::createMenuBar(this);

    // set default mode
    setMode_Select();
}
    

MainWindow::~MainWindow() {
}



// Implementation of setters for different modes
// They now forward the command to the Canvas
void MainWindow::setMode_Select() { 
    canvas->setTool(ToolType::Select);
    setWindowTitle("You have chosen mode--> SELECT");
}
void MainWindow::setMode_Circle() { 
    canvas->setTool(ToolType::Circle);
    setWindowTitle("You have chosen mode--> CIRCLE");
}
void MainWindow::setMode_Rect() { 
    canvas->setTool(ToolType::Rectangle);
    setWindowTitle("You have chosen mode--> RECTANGLE");
}
void MainWindow::setMode_Roundedrect() { 
    canvas->setTool(ToolType::RoundedRectangle);
    setWindowTitle("You have chosen mode--> ROUNDED RECTANGLE");
}
void MainWindow::setMode_Hexagon() { 
    canvas->setTool(ToolType::Hexagon);
    setWindowTitle("You have chosen mode--> HEXAGON");
}
void MainWindow::setMode_Line() { 
    canvas->setTool(ToolType::Line);
    setWindowTitle("You have chosen mode--> LINE");
}
void MainWindow::setMode_Text() { 
    canvas->setTool(ToolType::Text);
    setWindowTitle("You have chosen mode--> TEXT");
}
void MainWindow::setMode_Freehand() { 
    canvas->setTool(ToolType::FreehandSketch);
    setWindowTitle("You have chosen mode--> FREEHANDSKETCH");
}
void MainWindow::onSave_Trigger() {
    // 1. Open a "Save File" dialog box
    QString fileName = QFileDialog::getSaveFileName(
        this, 
        "Save Drawing", 
        "", 
        "SVG Files (*.svg)"
    );

    // user press cancel ;  u do nothing ;)
    if (fileName.isEmpty()) {
        return;
    }

    // ensure file ends with .svg
    if (!fileName.endsWith(".svg", Qt::CaseInsensitive)) {
        fileName += ".svg";
    }

    // save file
    //canvas uses std string so convert from QString to std::string 
    canvas->saveToFile(fileName.toStdString());

    // tell a success message
    QMessageBox::information(this, "Success", "File saved successfully!");
}

void MainWindow::onLoad_Trigger() {
    // 1. Open a "Open File" dialog box
    QString fileName = QFileDialog::getOpenFileName(
        this, 
        "Open Drawing", 
        "", 
        "SVG Files (*.svg)"
    );

    // user press cancel ;  u do nothing ;)
    if (fileName.isEmpty()) {
        return;
    }

    // load file
    canvas->loadFromFile(fileName.toStdString());

    // tell a success message
    QMessageBox::information(this, "Success", "File loaded successfully!");
}

void MainWindow::onNew() {
    // Now this works because 'currentFile' is in the header!
    currentFilename= ""; 
    
    // Clear the canvas
    if (canvas) {
        canvas->clear_new(); // Make sure your Canvas.h has a 'clear()' function
    }
}

void MainWindow::onSave_As() {
    QString fileName = QFileDialog::getSaveFileName(
        this, 
        "Save Drawing As", 
        "", 
        "SVG Files (*.svg)"
    );

    // do nothing if file is empty
    if (fileName.isEmpty()) {
        return;
    }

    // ensure .svg extension
    if (!fileName.endsWith(".svg",Qt::CaseInsensitive)) {
        fileName += ".svg";
    }

    // change the current filename
    currentFilename = fileName;

    // save
    canvas->saveToFile(currentFilename.toStdString());
    QMessageBox::information(this, "Success", "File saved");
}