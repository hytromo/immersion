#include "mainwindow.h"
#include "SingleInstance.h"

#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QCoreApplication::setOrganizationName("hytromo");
    QCoreApplication::setOrganizationDomain("hytromo.github.io");
    QCoreApplication::setApplicationName("immersion");

    // Check for single instance
    SingleInstance singleInstance;
    
    if (!singleInstance.tryToRun()) {
        // Another instance is already running
        singleInstance.bringExistingInstanceToFront();
        return 0;
    }

    MainWindow w;
    w.show();
    
    return a.exec();
}
