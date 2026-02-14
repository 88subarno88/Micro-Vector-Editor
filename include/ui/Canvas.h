#ifndef CANVAS_H
#define CANVAS_H

#include <QWidget>
#include <QMouseEvent>
#include <QPointF>
#include "core/Diagram.h"
#include "ui/EditorTypes.h" 
#include "commands/CommandStack.h"
#include "commands/CreateCommand.h"

class Canvas : public QWidget {
    Q_OBJECT

public:
    explicit Canvas(QWidget *parent = nullptr);
    
    // Called by MainWindow to change the tool
    void setTool(ToolType tool);
    void saveToFile(const std::string& filename);
    void loadFromFile(const std::string& filename);
    void undo();
    void redo();
    void clear_new();

protected:
    // Override standard Qt events
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;


private:
    Diagram diagram;
    ToolType currentMode;
    CommandStack commandStack;
    std::unique_ptr<GraphicsObject> current_Shape;
    
    // Drawing State Variables
    bool isDrawing;
    QPointF start_Point;
    QPointF curr_Point;



    GraphicsObject* selected_Object = nullptr; // Pointer to the shape we are moving
    bool isDragging_Selection = false;         // Are we dragging a shape right now?
    QPointF last_Mousepos;                     // Where the mouse was in the previous frame?
    QPointF drag_StartPos;
    std::unique_ptr<GraphicsObject> clipboard;
};

#endif // CANVAS_H