#ifndef CANVAS_H
#define CANVAS_H

#include <QMouseEvent>
#include <QPointF>
#include <QWidget>

#include "core/Diagram.h"
#include "ops/CommandStack.h"
#include "parser/SVGParser.h"
#include "ui/EditorTypes.h"

class Canvas : public QWidget {
    Q_OBJECT

public:
    explicit Canvas(QWidget *parent = nullptr);

    // Called by MainWindow to change the tool
    void setTool(ToolType tool);
    void saveToFile(const std::string &filename);
    // Replaces the drawing with the file's contents; on error the drawing is left unchanged.
    tl::expected<void, ParseError> loadFromFile(const std::string &filename);
    void undo();
    void redo();
    void clearDrawing();

protected:
    // Override standard Qt events
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    // Runs `op` through the undo history and repaints
    void run(const vec::Operation &op);
    // The selected shape; nullptr if none, or if it has been deleted or undone
    GraphicsObject *selectedShape() const { return diagram.find(selectedId); }

    Diagram diagram;
    ToolType currentMode;
    vec::CommandStack commandStack;
    std::unique_ptr<GraphicsObject> currentShape;

    // Drawing State Variables
    bool isDrawing;
    QPointF startPoint;
    QPointF currentPoint;

    ShapeId selectedId;                // The selected shape; empty if none
    bool isDraggingSelection = false;  // Are we dragging a shape right now?
    QPointF lastMousePos;              // Where the mouse was in the previous frame?
    QPointF dragStartPos;
    std::unique_ptr<GraphicsObject> clipboard;
};

#endif  // CANVAS_H