#include "parser/SVGParser.h"
#include <fstream>
#include <iostream>

void SVGParser::load_SVG(const std::string& filename, Diagram& diagram) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error:file isnot opening " << filename << "\n";        // added error
        return;
    }

    std::string curr_line;
    while (std::getline(file,curr_line)){
        parse_Line(curr_line,diagram);
    }
    file.close();
    // std::cout<<"file is ssucessfully loaded into diagram"<<"\n" ;             //Debug
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