#include "core/Text.h"
#include "core/Renderer.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <string>

//Text class
//text is defined by baseline point (x, y),content of the text,font family and font size
Text::Text(double x, double y, std::string content)
    : GraphicsObject(x, y)   // (x, y) = top-left of bounding box
{
    text_ = content;          // set the text content
    font_size_= 16;           // default size is 16
    font_family_= "Arial";
    updateSize();
}

// Estimates the bounding box without a font library: in Arial, typical text averages
// about 0.6 x font size per character and a line is about 1.5 x font size tall.
void Text::updateSize() {
    setWidth(0.6 * font_size_ * text_.length());
    setHeight(1.5 * font_size_);
}

Text::~Text() {}

void Text::draw(Renderer& renderer) const {
    renderer.drawText({{getX(), getY()}, text_, font_family_, font_size_}, style());
}
/**
 * Convert text to SVG format.
 */
std::string Text::toSVG() const {
    std::ostringstream svg;  

    svg << "<text " << idAttribute();
    svg << "x=\"" << getX() << "\" ";
    svg << "y=\"" << getY() << "\" ";
    svg << "font-size=\"" << font_size_ << "\" ";
    svg << "font-family=\"" << font_family_<< "\" ";
    svg << "fill=\"" << getFillColor() << "\" ";
    svg << "stroke=\"" << getStrokeColor() << "\" ";
    svg << "stroke-width=\"" << getStrokeWidth() << "\"";
    svg << ">";
    for (const char c : text_) {  // escape characters that would break the XML
        switch (c) {
            case '<':  svg << "&lt;";   break;
            case '>':  svg << "&gt;";   break;
            case '&':  svg << "&amp;";  break;
            case '"':  svg << "&quot;"; break;
            case '\'': svg << "&apos;"; break;
            default:   svg << c;
        }
    }
    svg << "</text>";
    
    return svg.str();
    
}

//Create the deep copy of the object | used for copy pasting purpose 
std::unique_ptr<GraphicsObject> Text::clone() const {
    auto copy = std::make_unique<Text>(getX(), getY(),text_ );
    copy->setFrontSize(font_size_);
    
    // Copy visual properties from base class
    copy->setStrokeColor(getStrokeColor());
    copy->setFillColor(getFillColor());
    copy->setStrokeWidth(getStrokeWidth());
    
    return copy;
}


//get type,here "Text"
std::string Text::getType() const {
    return "Text";
}

//setters
void Text::setFrontSize(double size) {
    if (size <= 0) {
        throw std::invalid_argument("Font size must be positive.");
    }else{
        font_size_ =size;
        updateSize();
    }
}
void Text::setText(const std::string& content) {
    text_ =content;
    updateSize();
}

//move
void Text::move(double dx, double dy) {
    GraphicsObject::move(dx, dy);
}
//resize
void Text::scale_factor(double factor) {
    font_size_ *=factor; 
    
    // Safety check to ensure font size isnot beyond visiblity
    if (font_size_ < 1){
         font_size_ = 1;
    }
    updateSize();
}