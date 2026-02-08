#ifndef CANVAS_H
#define CANVAS_H

#include <QWidget>
#include <QMouseEvent>
#include <QPointF>
#include "core/Diagram.h"
#include "ui/EditorTypes.h" // Needed for ToolType

class Canvas : public QWidget {
    Q_OBJECT

public:
    explicit Canvas(QWidget *parent = nullptr);
    
    // Called by MainWindow to change the tool
    void setTool(ToolType tool);
    void saveToFile(const std::string& filename);

protected:
    // Override standard Qt events
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;

private:
    Diagram diagram;
    ToolType currentMode;
    
    // Drawing State Variables
    bool isDrawing;
    QPointF start_Point;
    QPointF curr_Point;


    GraphicsObject* selected_Object = nullptr; // Pointer to the shape we are moving
    bool isDragging_Selection = false;         // Are we dragging a shape right now?
    QPointF last_Mousepos;                     // Where the mouse was in the previous frame
};

#endif // CANVAS_H