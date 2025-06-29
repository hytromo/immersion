#include "mainwindow.h"
#include "SingleInstance.h"
#include "AppConfig.h"

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

    // Set application metadata using centralized configuration
    QCoreApplication::setOrganizationName(AppConfig::ORGANIZATION_NAME);
    QCoreApplication::setOrganizationDomain(AppConfig::ORGANIZATION_DOMAIN);
    QCoreApplication::setApplicationName(AppConfig::APPLICATION_NAME);
    QCoreApplication::setApplicationVersion(AppConfig::APPLICATION_VERSION);

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
        if (!mainWindow.isWindowOnScreen()) {
            mainWindow.centerWindowOnScreen();
        }
    });
    
    return app.exec();
}
