#ifndef WANT_TO_INCLUDE_TEXT_H
#define WANT_TO_INCLUDE_TEXT_H

#include "core/GraphicsObject.h"
#include <cmath>
#include <string>

/**
 * Text class 
 * ------- represents a string of text
 * ------- a text is defined by --->a)baseline point (x, y)
 *                              --->b) Content of the text
 *                              --->c) Font family 
 *                              --->d) Font size
 */
class Text : public GraphicsObject {
private:
    std::string text_;  //the content
    std::string font_family_; //font family
    double font_size_;//font size

public:
    // Constructor
    Text(double x, double y, std::string content);
    
    // Destructor
    virtual ~Text();
    
    // Implement pure virtual functions from GraphicsObject
    void draw(QPainter* painter) override;
    void move(double dx, double dy) override;
    std::string toSVG() const override;
    std::unique_ptr<GraphicsObject> clone() const override;
    std::string getType() const override;
    
    //  setters
    void setFrontSize(double size);
    void setText(const std::string& content);
     // getters
    double getFontSize() const{ return font_size_;}
    std::string getText() const{ return text_; }
    
    
};

#endif // WANT_TO_INCLUDE_TEXT_H