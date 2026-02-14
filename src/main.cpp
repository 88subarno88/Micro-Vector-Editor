#include <QApplication>
#include "ui/MainWindow.h"

int main(int input_count, char *input_vector[]) {

    // the Qt app
    QApplication app(input_count, input_vector);

    // create main window object
    MainWindow window;

    //showing the main window
    window.show();

    //opens window and keep it open until closed
    return app.exec();
}