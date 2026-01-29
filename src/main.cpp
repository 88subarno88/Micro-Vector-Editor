#include <iostream>
#include <memory>
#include <fstream>
#include "core/GraphicsObject.h"
#include "core/Circle.h"
#include "core/Rectangle.h"
#include "core/Line.h"
#include "core/Text.h"
#include "core/RoundedRectangle.h"
#include "core/Hexagon.h"
#include "core/FreehandSketch.h"


int main() {
    std::cout << "==========================================\n";
    std::cout << "Testing All  Classes \n";
    std::cout << "==========================================\n\n";
    
    // Creating a simple circle 
    std::cout << "Creating a circle at position (60, 60) with radius 40...\n";
    Circle myCircle(60, 60, 40);
    myCircle.setStrokeColor("blue");
    myCircle.setFillColor("yellow");
    myCircle.setStrokeWidth(2.0);
    
    std::cout << "Circle created with Center: (" << myCircle.getCenterX() 
              << ", " << myCircle.getCenterY() 
              << "), Radius: " << myCircle.getRadius() << "\n\n";\

    std::cout << "Creating a hexagon at position (50, 50) with radius 40...\n";
    Hexagon myHexagon(50, 50, 40);
    myHexagon.setStrokeColor("red");
    myHexagon.setFillColor("yellow");
    myHexagon.setStrokeWidth(2.0);
    
    std::cout << "hexagon created with Center: (" << myHexagon.getCenterX() 
              << ", " <<myHexagon.getCenterY() 
              << "), Radius: " << myHexagon.getRadius() << "\n\n";


    // Creating a simple rectangle
    std::cout << "Creating a rectangle...\n";
    Rectangle rect(100, 100, 80, 50);
    rect.setStrokeColor("red");
    rect.setFillColor("pink");
    rect.setStrokeWidth(2.0);
    
    std::cout << "Rectangle created at -> (" << rect.getX() << ", " << rect.getY() 
              << ") with size " << rect.getWidth() << "x" << rect.getHeight() << "\n\n";

    // Creating a simple line
    std::cout << "Creating a line...\n";
    Line myLine(50, 50, 200, 150);
    myLine.setStrokeColor("green");
    myLine.setStrokeWidth(3.0);
    
    std::cout << "Line created from (" << myLine.getX1() << ", " << myLine.getY1() 
              << ") to (" << myLine.getX2() << ", " << myLine.getY2() << ")\n\n";
    
    // Creating a rounded rectangle
    std::cout << "Creating a rounded rectangle...\n";
    RoundedRectangle roundedRect(250, 100, 100, 70, 15);
    roundedRect.setStrokeColor("darkgreen");
    roundedRect.setFillColor("lightgreen");
    roundedRect.setStrokeWidth(2.0);
    
    std::cout << "Rounded Rectangle created at -> (" << roundedRect.getX() << ", " << roundedRect.getY() 
              << ") with size " << roundedRect.getWidth() << "x" << roundedRect.getHeight() 
              << ", corner radius: " << roundedRect.getCornerRadius() << "\n\n";
    
    // Creating a text 
    std::cout << "Creating a text object...\n";
    Text myText(50, 300, "Hello SVG!");
    myText.setStrokeColor("purple");
    myText.setFrontSize(24);
    
    std::cout << "Text created at(" << myText.getX() << ", " << myText.getY() 
              << ") saying: \"" << myText.getText() << "\"\n\n";
    
    // Creating a freehand sketch
    std::cout<<"Creating a freehand sketch...\n";
    FreehandSketch mySketch;
    mySketch.setStrokeColor("black");
    mySketch.setStrokeWidth(2.0);
    //making some patterns
    mySketch.addPoint(10,10);
    mySketch.addPoint(20,30);
    mySketch.addPoint(40,25);
    mySketch.addPoint(60,40);
    mySketch.addPoint(80,20);   

    std::cout << "Freehand sketch created with " << mySketch.getPointCount() << " points.\n\n";
    
    // Converting in SVG format
    std::cout << "Converting to SVG format....\n";
    std::cout << "Circle --> " << myCircle.toSVG() << "\n";
    std::cout << "Rectangle --> " << rect.toSVG() << "\n";
    std::cout << "Line --> " << myLine.toSVG() << "\n";
    std::cout << "Rounded Rectangle --> " << roundedRect.toSVG() << "\n\n";
    std::cout << "Hexagon --> " << myHexagon.toSVG() << "\n";
    std::cout << "Text --> " << myText.toSVG() << "\n\n";
    std::cout << "Freehand Sketch --> " << mySketch.toSVG() << "\n\n";
    
    // Testing if points are inside or outside the circle
    std::cout << "Testing which points are inside the circle....\n";
    std::cout << "  Does (50, 50) inside the circle?  Ans-->" 
              << (myCircle.contains(50, 50) ? "Yes" : "No") << "\n";
    std::cout << "  Does (100, 100) inside the circle?   Ans-->" 
              << (myCircle.contains(100, 100) ? "Yes" : "No") << "\n";
    std::cout << "  Does  (70, 50) inside the circle?    Ans-->" 
              << (myCircle.contains(70, 50) ? "Yes" : "No") << " \n\n";
    
    // Testing if points are inside or outside the hexagon
    std::cout << "Testing which points are inside the hexagon....\n";
    std::cout << "  Does (50, 50) inside the hexagon?  Ans-->" 
              << (myHexagon.contains(50, 50) ? "Yes" : "No") << "\n";
    std::cout << "  Does (100, 100) inside the hexagon?   Ans-->" 
              << (myHexagon.contains(100, 100) ? "Yes" : "No") << "\n";
    std::cout << "  Does  (70, 50) inside the hexagon?    Ans-->" 
              << (myHexagon.contains(70, 50) ? "Yes" : "No") << " \n\n";

    
    
    // Testing if points are inside or outside the rectangle
    std::cout << "Testing which points are inside the rectangle....\n";
    std::cout << "  Does (140, 125) inside rectangle?  Ans-->" 
              << (rect.contains(140, 125) ? "Yes" : "No") << "\n";
    std::cout << "  Does (50, 50) inside rectangle? Ans --> " 
              << (rect.contains(50, 50) ? "Yes" : "No") << "\n\n";
    
    // Testing if points are near the line
    std::cout << "Testing which points are near the line....\n";
    std::cout << "  Is (125, 100) near line?  Ans-->" 
              << (myLine.contains(125, 100) ? "Yes" : "No") << "\n";
    std::cout << "  Is (300, 300) near line? Ans --> " 
              << (myLine.contains(300, 300) ? "Yes" : "No") << "\n\n";
    
    // Testing if points are inside or outside the rounded rectangle
    std::cout << "Testing which points are inside the rounded rectangle....\n";
    std::cout << "  Does (300, 135) inside rounded rectangle?  Ans-->" 
              << (roundedRect.contains(300, 135) ? "Yes" : "No") << "\n";
    std::cout << "  Does (400, 400) inside rounded rectangle? Ans --> " 
              << (roundedRect.contains(400, 400) ? "Yes" : "No") << "\n\n";
    
    // Testing if points are inside or outside the freehandsketch
    std::cout << "Testing which points are inside the freehandsketch....\n";
    std::cout << "  Does (30, 35) inside freehandsketch?  Ans-->" 
              << (mySketch.contains(30, 35) ? "Yes" : "No") << "\n";
    std::cout << "  Does (100, 100) inside freehandsketch? Ans --> " 
              << (mySketch.contains(100, 100) ? "Yes" : "No") << "\n\n";

    
    
    //  Changing the circle's properties
    std::cout << "Changing circle position and size...\n";
    std::cout << "Before change--> " << myCircle.toSVG() << "\n";
    myCircle.setCenterX(100);
    myCircle.setCenterY(100);
    myCircle.setRadius(30);
    std::cout << "After change -->  " << myCircle.toSVG() << "\n\n";

    //  Changing the hexagon's properties
    std::cout << "Changing hexagon position and size...\n";
    std::cout << "Before change--> " << myHexagon.toSVG() << "\n";
    myHexagon.setCenterX(100);
    myHexagon.setCenterY(100);
    myHexagon.setRadius(30);
    std::cout << "After change -->  " << myHexagon.toSVG() << "\n\n";

    //Changing the rectangle's properties
    std::cout << "Changing rectangle position and size...\n";
    std::cout << "Before change-->" << rect.toSVG() << "\n";
    rect.setX(50);
    rect.setY(200);
    rect.setWidth(120);
    rect.setHeight(60);
    std::cout << "After change -->   " << rect.toSVG() << "\n\n";

    // Changing the line's endpoints
    std::cout << "Changing line endpoints...\n";
    std::cout << "Before change--> " << myLine.toSVG() << "\n";
    myLine.setStartPoint(100, 50);
    myLine.setEndPoint(300, 200);
    std::cout << "After change -->  " << myLine.toSVG() << "\n\n";
    
    // Changing the rounded rectangle's properties
    std::cout << "Changing rounded rectangle corner radius...\n";
    std::cout << "Before change--> " << roundedRect.toSVG() << "\n";
    roundedRect.setCornerRadius(25);
    std::cout << "After change -->  " << roundedRect.toSVG() << "\n\n";

    // Changing text properties
    std::cout << "Changing text content...\n";
    std::cout << "Before change--> " << myText.toSVG() << "\n";
    myText.setText("my svg is  cool.");
    myText.setFrontSize(30);
    std::cout << "After change -->  " << myText.toSVG() << "\n\n";
    
    // Testing the circle clone functionality
    std::cout << "Making a copy of the circle...\n";
    auto circleCopy = myCircle.clone();
    std::cout << "Original circle--> " << myCircle.toSVG() << "\n";
    std::cout << "Copied circle-->  " << circleCopy->toSVG() << "\n\n";

    // Testing the hexagon clone functionality
    std::cout << "Making a copy of the hexagon...\n";
    auto HexagonCopy = myHexagon.clone();
    std::cout << "Original hexagon--> " << myHexagon.toSVG() << "\n";
    std::cout << "Copied hexagon-->  " << HexagonCopy->toSVG() << "\n\n";

    // Testing the rectangle clone functionality
    std::cout << "Making a copy of the rectangle...\n";
    auto rectCopy = rect.clone();
    std::cout << "Original rectangle --> " << rect.toSVG() << "\n";
    std::cout << "Copied rectangle -->  " << rectCopy->toSVG() << "\n\n";

    // Testing the line clone functionality
    std::cout << "Making a copy of the line...\n";
    auto lineCopy = myLine.clone();
    std::cout << "Original line --> " << myLine.toSVG() << "\n";
    std::cout << "Copied line -->  " << lineCopy->toSVG() << "\n\n";
    
    // Testing the rounded rectangle clone functionality
    std::cout << "Making a copy of the rounded rectangle...\n";
    auto roundedRectCopy = roundedRect.clone();
    std::cout << "Original rounded rectangle --> " << roundedRect.toSVG() << "\n";
    std::cout << "Copied rounded rectangle -->  " << roundedRectCopy->toSVG() << "\n\n";

    //Testing the text clone functionality
    std::cout << "Making a copy of the text...\n";
    auto textCopy = myText.clone();
    std::cout << "Original text --> " << myText.toSVG() << "\n";
    std::cout << "Copied text --> " << textCopy->toSVG() << "\n\n";

    // Testing freehandsketch clone
    std::cout << "Making a copy of the freehand sketch...\n";
    auto sketchCopy = mySketch.clone();
    std::cout << "Original sketch --> " << mySketch.toSVG() << "\n";
    std::cout << "Copied sketch -->  " << sketchCopy->toSVG() << "\n\n";
    
    
    // Check if polymorphism works for circle
    std::cout << "Testing polymorphism for circle....\n";
    GraphicsObject* hexPtr = &myCircle;
    std::cout << "Shape type -->" << hexPtr->getType() << "\n";
    std::cout << "SVG output -->" << hexPtr->toSVG() << "\n\n";

    // Check if polymorphism works for hexagon
    std::cout << "Testing polymorphism for hexagon....\n";
    GraphicsObject* basePtr = &myHexagon;
    std::cout << "Shape type -->" << basePtr->getType() << "\n";
    std::cout << "SVG output -->" << basePtr->toSVG() << "\n\n";

    //Check if polymorphism works for rectangle
    std::cout << "Testing polymorphism with rectangle ....\n";
    GraphicsObject* objPtr = &rect;
    std::cout << "Shape type --> " << objPtr->getType() << "\n";
    std::cout << "SVG output --> " << objPtr->toSVG() << "\n\n";

    // Check if polymorphism works for line
    std::cout << "Testing polymorphism with line....\n";
    GraphicsObject* linePtr = &myLine;
    std::cout << "Shape type --> " << linePtr->getType() << "\n";
    std::cout << "SVG output --> " << linePtr->toSVG() << "\n\n";
    
    // Check if polymorphism works for rounded rectangle
    std::cout << "Testing polymorphism with rounded rectangle....\n";
    GraphicsObject* roundedPtr = &roundedRect;
    std::cout << "Shape type --> " << roundedPtr->getType() << "\n";
    std::cout << "SVG output --> " << roundedPtr->toSVG() << "\n\n";

    // Check if polymorphism works for text
    std::cout << "Testing polymorphism with text....\n";
    GraphicsObject* txtPtr = &myText;
    std::cout << "Shape type --> " <<txtPtr->getType() << "\n";
    std::cout << "SVG output --> " <<txtPtr->toSVG() << "\n\n";

    // Check if polymorphism works for freehand
    std::cout << "Testing polymorphism with freehand sketch....\n";
    GraphicsObject* sketchPtr = &mySketch;
    std::cout << "Shape type --> " << sketchPtr->getType() << "\n";
    std::cout << "SVG output --> " << sketchPtr->toSVG() << "\n\n";

    
    
    // Creating an SVG file with circles, rectangles ,lines,...etc
    std::cout << "Generating an SVG file with all shapes...\n";
    std::ofstream file("test_shapes.svg");
    
    file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
    file << "<svg width=\"400\" height=\"400\" xmlns=\"http://www.w3.org/2000/svg\">\n";
    
    // Adding circles
    Circle c1(50, 50, 40);
    c1.setStrokeColor("blue");
    c1.setFillColor("yellow");
    c1.setStrokeWidth(2);
    file << "  " << c1.toSVG() << "\n";
    
    Circle c2(350, 350, 30);
    c2.setStrokeColor("purple");
    c2.setFillColor("lavender");
    c2.setStrokeWidth(2);
    file << "  " << c2.toSVG() << "\n";

    // Adding hexagon
    Hexagon h1(50, 50, 40);
    h1.setStrokeColor("blue");
    h1.setFillColor("yellow");
    h1.setStrokeWidth(2);
    file << "  " << h1.toSVG() << "\n";
    
    Hexagon h2(350, 350, 30);
    h2.setStrokeColor("purple");
    h2.setFillColor("lavender");
    h2.setStrokeWidth(2);
    file << "  " << h2.toSVG() << "\n";

    // Adding rectangles
    Rectangle r1(150, 100, 80, 60);
    r1.setStrokeColor("red");
    r1.setFillColor("pink");
    r1.setStrokeWidth(2);
    file << "  " << r1.toSVG() << "\n";
    
    Rectangle r2(50, 250, 100, 40);
    r2.setStrokeColor("orange");
    r2.setFillColor("lightyellow");
    file << "  " << r2.toSVG() << "\n";

    // Adding lines
    Line l1(50, 50, 350, 350);
    l1.setStrokeColor("green");
    l1.setStrokeWidth(3);
    file << "  " << l1.toSVG() << "\n";
    
    Line l2(350, 50, 50, 350);
    l2.setStrokeColor("darkblue");
    l2.setStrokeWidth(2);
    file << "  " << l2.toSVG() << "\n";
    
    // Adding rounded rectangles
    RoundedRectangle rr1(200, 250, 120, 80, 20);
    rr1.setStrokeColor("darkgreen");
    rr1.setFillColor("lightgreen");
    rr1.setStrokeWidth(2);
    file << " " << rr1.toSVG() << "\n";
    
    RoundedRectangle rr2(250, 50, 100, 60, 15);
    rr2.setStrokeColor("brown");
    rr2.setFillColor("beige");
    rr2.setStrokeWidth(2);
    file << "  " << rr2.toSVG() << "\n";

    // Adding text
    Text t1(220, 250, "Nightowl is dead");
    t1.setStrokeColor("darkred");
    t1.setFrontSize(20);
    file << "  " << t1.toSVG() << "\n";
    
    Text t2(100, 30, "May rivers bless u");
    t2.setStrokeColor("navy");
    t2.setFrontSize(16);
    file << "  " << t2.toSVG() << "\n";

    // Adding freehand sketches
    FreehandSketch fs1;
    fs1.setStrokeColor("darkviolet");
    fs1.setStrokeWidth(3);
    fs1.addPoint(250, 120);
    fs1.addPoint(270, 150);
    fs1.addPoint(250, 180);
    fs1.addPoint(270, 210);
    fs1.addPoint(250, 240);
    file << "  " << fs1.toSVG() << "\n";
    
    FreehandSketch fs2;
    fs2.setStrokeColor("olive");
    fs2.setStrokeWidth(2);
    fs2.addPoint(350, 300);
    fs2.addPoint(360, 310);
    fs2.addPoint(370, 305);
    fs2.addPoint(380, 315);
    file << "  " << fs2.toSVG() << "\n";
    
    file << "</svg>\n";
    file.close();


    std::cout << "==========================================\n";
    std::cout << "All tests are done!\n";
    std::cout << "Check if all classes work!\n";
    std::cout << "==========================================\n\n";
    
    return 0;
}