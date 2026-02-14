#include "core/Text.h"
#include <sstream>
#include <iomanip>
#include <cmath>
#include <string>
#include <QFont>
#include <QFontMetrics>
#include <QPainterPath>
#include <QFontMetricsF>

//Text class
//text is defined by baseline point (x, y),content of the text,font family and font size
Text::Text(double x, double y, std::string content)
    : GraphicsObject(
        x,           // x (top-left of bounding box)
        y,           // y (top-left of bounding box)
        80,          // default width (diameter)
        20           // default height (diameter)
){
    text_ = content;          // set the text content
    font_size_= 16;           // default size is 16
    font_family_= "Arial";
    double width_guess = text_.length()*20;  // Rough guess
    setWidth(width_guess);                   //default
    setHeight(40);                           //default
}    

Text::~Text() {}

// Drawing the text using QPainter.
void Text::draw(QPainter* painter) {
    if (!painter) return;

    QString family = QString::fromStdString(font_family_);
    if (family.isEmpty()) family = "Arial";                  // default fontfamily
    
    QFont font(family);
    font.setPointSizeF(font_size_);
    painter->setFont(font);

    // convert text to a Vector Path 
    QPainterPath path;
    QFontMetricsF fm(font);
    
    // addText uses the text's "baseline" (bottom). 
    path.addText(getX(), getY() + fm.ascent(), font, QString::fromStdString(text_));

    // fill inside text
    QBrush brush{QColor(QString::fromStdString(getFillColor()))};
    painter->fillPath(path, brush);

    // draw stroke
    QPen pen{QColor(QString::fromStdString(getStrokeColor()))};
    
    // apply thickness
    pen.setWidthF(getStrokeWidth()); 
    
    painter->strokePath(path, pen);
}
/**
 * Convert text to SVG format.
 */
std::string Text::toSVG() const {
    std::ostringstream svg;  

    svg << "<text ";
    svg << "x=\"" << getX() << "\" ";
    svg << "y=\"" << getY() << "\" ";
    svg << "font-size=\"" << font_size_ << "\" ";
    svg << "font-family=\"" << font_family_<< "\" ";
    svg << "fill=\"" << getStrokeColor() << "\"";
    svg << ">";
    svg <<text_;
    svg << "</text>";
    
    return svg.str();
    
}

//Create the deep copy of the object | used for copy pasting purpose 
std::unique_ptr<GraphicsObject> Text::clone() const {
    auto copy = std::make_unique<Text>(getX(), getY(),text_ );
    
    // Copy visual properties from base class
    copy->setStrokeColor(getStrokeColor());
    copy->setFillColor(getFillColor());
    copy->setStrokeWidth(getStrokeWidth());
    copy->setSelected(isSelected());
    
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
    }
}
void Text::setText(const std::string& content) {
    text_ =content;
    double new_width= content.length()*10;  // Rough guess
    setWidth(new_width);

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

    // measure the NEW size of the text and keep the fontstyle as before
    QFont font("Arial", static_cast<int>(font_size_)); 
    QFontMetrics metrics(font);
    
    // Calculate new dimensions based on the text string
    QString qText = QString::fromStdString(text_); 
    
    double newWidth = metrics.horizontalAdvance(qText); 
    double newHeight = metrics.height();

    // update object dimensions
    // a small buffer (+7) is added to prevent the last letter from being clipped
    setWidth(newWidth +7); 
    setHeight(newHeight);
}