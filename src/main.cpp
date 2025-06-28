#include "mainwindow.h"
#include "SingleInstance.h"

#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>
#include <QGuiApplication>
#include <QScreen>
#include <QRect>

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
    
    // Start listening for messages from other instances
    singleInstance.startListening();
    
    // Connect the signal to bring window to front
    QObject::connect(&singleInstance, &SingleInstance::bringToFrontRequested, [&w]() {
        w.setWindowState(Qt::WindowNoState);
        w.show();
        w.raise();
        w.activateWindow();
        w.setFocus();
        
        // Ensure it's visible on the current screen
        QScreen *currentScreen = QGuiApplication::primaryScreen();
        if (currentScreen) {
            QRect screenGeometry = currentScreen->geometry();
            QRect windowGeometry = w.geometry();
            
            // If window is outside visible area, center it
            if (!screenGeometry.intersects(windowGeometry)) {
                w.move(
                    screenGeometry.center().x() - windowGeometry.width() / 2,
                    screenGeometry.center().y() - windowGeometry.height() / 2
                );
            }
        }
    });
    
    return a.exec();
}
