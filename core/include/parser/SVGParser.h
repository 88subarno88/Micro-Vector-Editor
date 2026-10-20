#ifndef SVGPARSER_H
#define SVGPARSER_H

#include <string>
#include <vector>
#include <memory>
#include "core/Diagram.h" 

class SVGParser {
public:
    //read files and load into diagram
    static void load_SVG(const std::string& filename, Diagram& diagram);

private:
    // parse single line
    static void parse_Line(const std::string& line, Diagram& diagram);
    
    // extract a value string
    static std::string get_Attribute(const std::string& line, const std::string& key);
};

#endif