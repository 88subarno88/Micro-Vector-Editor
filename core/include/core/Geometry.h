#ifndef GEOMETRY_H
#define GEOMETRY_H

#include <string>

// Plain data passed from shapes to a Renderer. No drawing library types.

struct Point {
    double x = 0.0;
    double y = 0.0;
};

// Colours are SVG colour strings ("red", "#ff0000"); fill "none" means not filled.
struct Style {
    std::string stroke = "black";
    std::string fill = "white";
    double strokeWidth = 1.0;
};

struct RectGeom {
    double x = 0.0;  // top-left corner
    double y = 0.0;
    double width = 0.0;
    double height = 0.0;
    double cornerRadius = 0.0;  // 0 = sharp corners
};

struct EllipseGeom {
    Point center;
    double rx = 0.0;
    double ry = 0.0;
};

struct TextGeom {
    Point topLeft;
    std::string text;
    std::string fontFamily;
    double fontSize = 16.0;
};

#endif // GEOMETRY_H
