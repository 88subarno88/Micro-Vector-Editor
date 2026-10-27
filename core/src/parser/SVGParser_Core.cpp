#include "parser/SVGParser.h"
#include <cctype>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iostream>
#include <iterator>

void SVGParser::load_SVG(const std::string& filename, Diagram& diagram) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: cannot open " << filename << "\n";
        return;
    }
    const std::string doc{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};

    size_t pos = 0;
    for (auto tag = nextTag(doc, pos); !tag.empty(); tag = nextTag(doc, pos)) {
        // Text between this tag and the next one, e.g. the "Hi" of <text>Hi</text>.
        const auto content = std::string_view(doc).substr(pos, doc.find('<', pos) - pos);
        parse_Tag(tag, content, diagram);
    }
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

std::string_view SVGParser::nextTag(std::string_view doc, size_t& pos) {
    while (true) {
        const size_t start = doc.find('<', pos);
        if (start == std::string_view::npos) break;

        if (doc.compare(start, 4, "<!--") == 0) {
            const size_t end = doc.find("-->", start + 4);
            if (end == std::string_view::npos) break;
            pos = end + 3;
            continue;
        }

        // Find the closing '>', ignoring any inside quoted attribute values.
        char quote = 0;
        size_t i = start + 1;
        for (; i < doc.size(); ++i) {
            const char c = doc[i];
            if (quote != 0) {
                if (c == quote) quote = 0;
            } else if (c == '"' || c == '\'') {
                quote = c;
            } else if (c == '>') {
                break;
            }
        }
        if (i >= doc.size()) break;

        pos = i + 1;
        if (doc[start + 1] == '?' || doc[start + 1] == '!') continue;  // <?xml ...?>, <!DOCTYPE>
        return doc.substr(start, pos - start);
    }
    pos = doc.size();
    return {};
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
