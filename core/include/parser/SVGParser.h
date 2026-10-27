#ifndef SVGPARSER_H
#define SVGPARSER_H

#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#include <memory>
#include "core/Diagram.h"

class SVGParser {
public:
    using Attributes = std::unordered_map<std::string, std::string>;

    //read files and load into diagram
    static void load_SVG(const std::string& filename, Diagram& diagram);

    // Parses the name="value" pairs of one opening tag, e.g. <rect x="1" rx='2'/>.
    // Stops at the closing '>'; on malformed input returns the pairs read so far.
    static Attributes parseAttributes(std::string_view tag);

    // Parses the leading number of `text` ("10", " +1.5e2", "10px").
    // Returns `fallback` for empty, non-numeric or non-finite input. Never throws.
    static double parseNumber(std::string_view text, double fallback);

private:
    // parse single line
    static void parse_Line(const std::string& line, Diagram& diagram);
    
    // extract a value string
    static std::string get_Attribute(const std::string& line, const std::string& key);
};

#endif