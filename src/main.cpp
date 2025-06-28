#include "mainwindow.h"
#include "SingleInstance.h"

#include <QApplication>
#include <QCoreApplication>
#include <QMessageBox>
#include <QGuiApplication>
#include <QScreen>
#include <QRect>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application metadata
    QCoreApplication::setOrganizationName("hytromo");
    QCoreApplication::setOrganizationDomain("hytromo.github.io");
    QCoreApplication::setApplicationName("immersion");
    QCoreApplication::setApplicationVersion("0.1");

    // Check for single instance
    SingleInstance singleInstance;
    
    if (!singleInstance.tryToRun()) {
        // Another instance is already running
        qDebug() << "Another instance is already running, bringing it to front";
        singleInstance.bringExistingInstanceToFront();
        return 0;
    }

    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();
    
    // Start listening for messages from other instances
    singleInstance.startListening();
    
    // Connect the signal to bring window to front
    QObject::connect(&singleInstance, &SingleInstance::bringToFrontRequested, [&mainWindow]() {
        qDebug() << "Bringing existing window to front";
        
        // Restore window state
        mainWindow.setWindowState(Qt::WindowNoState);
        mainWindow.show();
        mainWindow.raise();
        mainWindow.activateWindow();
        mainWindow.setFocus();
        
        // Ensure it's visible on the current screen
        if (const QScreen *currentScreen = QGuiApplication::primaryScreen()) {
            const QRect screenGeometry = currentScreen->geometry();
            const QRect windowGeometry = mainWindow.geometry();
            
            // If window is outside visible area, center it
            if (!screenGeometry.intersects(windowGeometry)) {
                int newX = screenGeometry.center().x() - windowGeometry.width() / 2;
                int newY = screenGeometry.center().y() - windowGeometry.height() / 2;
                
                // Ensure window doesn't go off-screen
                newX = qMax(screenGeometry.left(), qMin(newX, screenGeometry.right() - windowGeometry.width()));
                newY = qMax(screenGeometry.top(), qMin(newY, screenGeometry.bottom() - windowGeometry.height()));
                
                mainWindow.move(newX, newY);
            }
        }
    });
    
    return app.exec();
}
