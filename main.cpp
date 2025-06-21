#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("hytromo");
    QCoreApplication::setOrganizationDomain("hytromo.github.io");
    QCoreApplication::setApplicationName("immersion");

    MainWindow w;
    w.show();
    return a.exec();
}
