#include "parser/SVGParser.h"
#include <cctype>
#include <charconv>
#include <cmath>
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

SVGParser::Attributes SVGParser::parseAttributes(std::string_view tag) {
    Attributes attributes;
    const auto isSpace = [](char c) { return std::isspace(static_cast<unsigned char>(c)) != 0; };
    size_t i = 0;
    const auto skipSpaces = [&] { while (i < tag.size() && isSpace(tag[i])) ++i; };

    // Skip '<' and the tag name.
    while (i < tag.size() && tag[i] == '<') ++i;
    while (i < tag.size() && !isSpace(tag[i]) && tag[i] != '>' && tag[i] != '/') ++i;

    while (true) {
        skipSpaces();
        if (i >= tag.size() || tag[i] == '>' || tag[i] == '/') break;

        const size_t nameStart = i;
        while (i < tag.size() && !isSpace(tag[i]) && tag[i] != '=' && tag[i] != '>') ++i;
        const std::string_view name = tag.substr(nameStart, i - nameStart);

        skipSpaces();
        if (i >= tag.size() || tag[i] != '=') break;
        ++i;
        skipSpaces();
        if (i >= tag.size() || (tag[i] != '"' && tag[i] != '\'')) break;

        const char quote = tag[i++];
        const size_t valueEnd = tag.find(quote, i);
        if (valueEnd == std::string_view::npos) break;

        attributes.emplace(name, tag.substr(i, valueEnd - i));
        i = valueEnd + 1;
    }
    return attributes;
}

double SVGParser::parseNumber(std::string_view text, double fallback) {
    while (!text.empty() && std::isspace(static_cast<unsigned char>(text.front()))) {
        text.remove_prefix(1);
    }
    if (!text.empty() && text.front() == '+') text.remove_prefix(1);

    double value = 0.0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc() || !std::isfinite(value)) return fallback;
    return value;
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