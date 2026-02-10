#include "parser/SVGParser.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <limits>
#include <cmath>

//include all shapes
#include "core/Circle.h"
#include "core/Rectangle.h"
#include "core/Line.h"
#include "core/Hexagon.h"
#include "core/RoundedRectangle.h"
#include "core/Text.h"
#include "core/FreehandSketch.h"

void SVGParser::load_SVG(const std::string& filename, Diagram& diagram) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error:file isnot opening " << filename << "\n";
        return;
    }

    std::string curr_line;
    while (std::getline(file,curr_line)){
        parse_Line(curr_line,diagram);
    }
    file.close();
    std::cout<<"file is ssucessfully loaded into diagram"<<"\n" ;
}

std::string SVGParser::get_Attribute(const std::string& curr_line, const std::string& word) {
    std::string search_word = word + "=\"";
    size_t start_pos = curr_line.find(search_word);
    if (start_pos == std::string::npos) return ""; // Not found
    start_pos += search_word.length();
    size_t end_pos = curr_line.find("\"", start_pos);
    if (end_pos == std::string::npos) return ""; // Malformed
    return curr_line.substr(start_pos, end_pos - start_pos);
}

enum class WhatdaShape {
    Rect,
    Circle,
    Line,
    Text,
    Hexagon,
    Freehand,   
    Unknown
};

// function to convert string tag to Enum
WhatdaShape identifyTag(const std::string& line) {
    if (line.find("<rect") != std::string::npos) return WhatdaShape::Rect;
    if (line.find("<circle") != std::string::npos) return WhatdaShape::Circle;
    if (line.find("<line") != std::string::npos) return WhatdaShape::Line;
    if (line.find("<text") != std::string::npos) return WhatdaShape::Text;
    if (line.find("<polygon") != std::string::npos) return WhatdaShape::Hexagon;
    if (line.find("<path") != std::string::npos) return WhatdaShape::Freehand;
    return WhatdaShape::Unknown;
}

void SVGParser::parse_Line(const std::string& line, Diagram& diagram) {
    WhatdaShape tag = identifyTag(line);
    switch (tag) {
        case WhatdaShape::Rect: {
            double x=std::stod(get_Attribute(line,"x"));
            double y=std::stod(get_Attribute(line,"y"));
            double w=std::stod(get_Attribute(line,"width"));
            double h=std::stod(get_Attribute(line,"height"));
            std::string cluestr = get_Attribute(line,"rx");//handled regular rectangle and rounded rectangle with the cluestr

            if (!cluestr.empty() && std::stod(cluestr) > 0) {
                auto shape = std::make_unique<RoundedRectangle>(x, y, w, h, std::stod(cluestr));
                shape->setStrokeColor(get_Attribute(line, "stroke"));
                shape->setFillColor(get_Attribute(line, "fill"));
                shape->setStrokeWidth(std::stod(get_Attribute(line,"stroke-width")));
                diagram.addObject(std::move(shape));
            } else {
                auto shape = std::make_unique<Rectangle>(x, y, w, h);
                shape->setStrokeColor(get_Attribute(line,"stroke"));
                shape->setFillColor(get_Attribute(line,"fill"));
                shape->setStrokeWidth(std::stod(get_Attribute(line,"stroke-width")));
                diagram.addObject(std::move(shape));
            }
            break;
        }

        case WhatdaShape::Circle: {
            double cx=std::stod(get_Attribute(line,"cx"));
            double cy=std::stod(get_Attribute(line,"cy"));
            double r=std::stod(get_Attribute(line,"r"));

            auto shape=std::make_unique<Circle>(cx,cy,r);
            shape->setStrokeColor(get_Attribute(line,"stroke"));
            shape->setFillColor(get_Attribute(line,"fill"));
            shape->setStrokeWidth(std::stod(get_Attribute(line,"stroke-width")));
            diagram.addObject(std::move(shape));
            break;
        }


        case WhatdaShape::Line: {
            double x1=std::stod(get_Attribute(line,"x1"));
            double y1=std::stod(get_Attribute(line,"y1"));
            double x2=std::stod(get_Attribute(line,"x2"));
            double y2=std::stod(get_Attribute(line,"y2"));

            auto shape=std::make_unique<Line>(x1,y1,x2,y2);
            shape->setStrokeColor(get_Attribute(line,"stroke"));
            shape->setStrokeWidth(std::stod(get_Attribute(line,"stroke-width")));
            diagram.addObject(std::move(shape));
            break;
        }

        case WhatdaShape::Text:{
            double x = std::stod(get_Attribute(line,"x"));
            double y = std::stod(get_Attribute(line,"y"));

            size_t startContent = line.find(">") + 1;
            size_t endContent = line.find("</text>");
            if (startContent < endContent) {
                std::string content = line.substr(startContent, endContent - startContent);
                auto shape = std::make_unique<Text>(x,y,content);
                shape->setStrokeColor(get_Attribute(line,"fill"));
                diagram.addObject(std::move(shape));
            }
            break;
        }

        case WhatdaShape::Hexagon: {
            std::string pointsStr = get_Attribute(line,"points");
            std::stringstream ss(pointsStr);
            double val, min_X = 1e9, max_X = -1e9, min_Y = 1e9, max_Y = -1e9;
            std::vector<double> coords;

            while (ss >> val) {
                coords.push_back(val);
                if (ss.peek() == ',' || ss.peek() == ' ') ss.ignore();
            }

            for (size_t i = 0; i < coords.size(); i += 2) {
                double px = coords[i];
                double py = coords[i + 1];
                if (px < min_X) min_X = px;
                if (px > max_X) max_X = px;
                if (py < min_Y) min_Y = py;
                if (py > max_Y) max_Y = py;
            }

            double cx = (min_X+max_X)/2.0;
            double cy = (min_Y+max_Y)/2.0;
            double radius = (max_X-min_X)/2.0;

            auto shape = std::make_unique<Hexagon>(cx, cy, radius);
            shape->setStrokeColor(get_Attribute(line,"stroke"));
            shape->setFillColor(get_Attribute(line,"fill"));
            shape->setStrokeWidth(std::stod(get_Attribute(line,"stroke-width")));
            diagram.addObject(std::move(shape));
            break;
        }

        case WhatdaShape::Freehand: {
            auto sketch = std::make_unique<FreehandSketch>();
            std::string d = get_Attribute(line, "d");
            std::stringstream ss(d);
            char cmd;
            double px, py;

            while (ss >> cmd) {
                if (cmd == 'M' || cmd == 'L') {
                    if (ss >> px >> py) {
                        sketch->addPoint(px, py);
                    }
                }
            }
            sketch->setStrokeColor(get_Attribute(line, "stroke"));
            sketch->setStrokeWidth(std::stod(get_Attribute(line, "stroke-width")));
            diagram.addObject(std::move(sketch));
            break;
        }

        case WhatdaShape::Unknown:
        default:
            break;
    }
}