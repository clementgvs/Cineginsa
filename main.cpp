#include <QApplication>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setOrganizationName("Cineginsa");
    app.setApplicationName("Cineginsa");
    app.setStyle("Fusion");

    MainWindow w;
    w.show();
    return app.exec();
}
