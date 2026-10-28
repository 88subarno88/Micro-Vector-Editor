#include "parser/SVGParser.h"
#include <algorithm>
#include <cctype>
#include <charconv>
#include <cmath>
#include <fstream>
#include <iterator>

namespace {

// Builds an error pointing at byte `offset` of `doc`.
ParseError errorAt(std::string_view doc, size_t offset, std::string message) {
    const std::string_view before = doc.substr(0, offset);
    const size_t newline = before.rfind('\n');
    const size_t lineStart = newline == std::string_view::npos ? 0 : newline + 1;
    return {static_cast<int>(std::count(before.begin(), before.end(), '\n')) + 1,
            static_cast<int>(offset - lineStart) + 1, std::move(message)};
}

}  // namespace

tl::expected<Diagram, ParseError> SVGParser::load_SVG(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return tl::unexpected(ParseError{0, 0, "cannot open " + filename});

    const std::string doc{std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>()};
    return parse_SVG(doc);
}

tl::expected<Diagram, ParseError> SVGParser::parse_SVG(std::string_view doc) {
    Diagram diagram;
    size_t pos = 0;
    for (auto tag = nextTag(doc, pos); !tag.empty(); tag = nextTag(doc, pos)) {
        // Text between this tag and the next one, e.g. the "Hi" of <text>Hi</text>.
        const auto content = doc.substr(pos, doc.find('<', pos) - pos);
        parse_Tag(tag, content, diagram);
    }
    if (pos < doc.size()) {
        const bool comment = doc.compare(pos, 4, "<!--") == 0;
        return tl::unexpected(errorAt(doc, pos, comment ? "unclosed comment" : "unclosed tag"));
    }
    return diagram;
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
        if (start == std::string_view::npos) {
            pos = doc.size();
            return {};
        }

        if (doc.compare(start, 4, "<!--") == 0) {
            const size_t end = doc.find("-->", start + 4);
            if (end == std::string_view::npos) {
                pos = start;
                return {};
            }
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
        if (i >= doc.size()) {
            pos = start;
            return {};
        }

        pos = i + 1;
        if (doc[start + 1] == '?' || doc[start + 1] == '!') continue;  // <?xml ...?>, <!DOCTYPE>
        return doc.substr(start, pos - start);
    }
}

std::string SVGParser::decodeEntities(std::string_view text) {
    static const std::pair<std::string_view, char> entities[] = {
        {"&lt;", '<'}, {"&gt;", '>'}, {"&amp;", '&'}, {"&quot;", '"'}, {"&apos;", '\''},
    };
    std::string out;
    out.reserve(text.size());
    for (size_t i = 0; i < text.size();) {
        const auto match = std::find_if(std::begin(entities), std::end(entities), [&](const auto& e) {
            return text.compare(i, e.first.size(), e.first) == 0;
        });
        if (match != std::end(entities)) {
            out += match->second;
            i += match->first.size();
        } else {
            out += text[i++];
        }
    }
    return out;
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
