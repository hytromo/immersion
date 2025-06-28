#include "SingleInstance.h"
#include <QApplication>
#include <QWindow>
#include <QScreen>
#include <QDebug>
#include <QTimer>
#include <QWidget>
#include <QMainWindow>

const QString SingleInstance::SHARED_MEMORY_KEY = "immersion_single_instance";

SingleInstance::SingleInstance(QObject *parent)
    : QObject(parent)
    , m_sharedMemory(nullptr)
{
}

SingleInstance::~SingleInstance()
{
    if (m_sharedMemory) {
        m_sharedMemory->detach();
        delete m_sharedMemory;
    }
}

bool SingleInstance::isAnotherInstanceRunning()
{
    QSharedMemory tempSharedMemory(SHARED_MEMORY_KEY);
    if (tempSharedMemory.attach()) {
        tempSharedMemory.detach();
        return true;
    }
    return false;
}

bool SingleInstance::tryToRun()
{
    m_sharedMemory = new QSharedMemory(SHARED_MEMORY_KEY);
    
    if (m_sharedMemory->attach()) {
        // Another instance is already running
        m_sharedMemory->detach();
        delete m_sharedMemory;
        m_sharedMemory = nullptr;
        return false;
    }
    
    // Create the shared memory segment
    if (!m_sharedMemory->create(SHARED_MEMORY_SIZE)) {
        // Failed to create shared memory, another instance might be running
        delete m_sharedMemory;
        m_sharedMemory = nullptr;
        return false;
    }
    
    return true;
}

void SingleInstance::bringExistingInstanceToFront()
{
    // Find all top-level widgets (windows)
    QWidgetList widgets = QApplication::topLevelWidgets();
    
    for (QWidget *widget : widgets) {
        QMainWindow *mainWindow = qobject_cast<QMainWindow*>(widget);
        if (mainWindow) {
            // Check if this is our application window by looking at the window title
            QString windowTitle = mainWindow->windowTitle();
            if (windowTitle.contains("Immersion", Qt::CaseInsensitive) ||
                windowTitle.contains("immersion", Qt::CaseInsensitive) ||
                windowTitle.isEmpty()) { // MainWindow might not have a title set yet
                
                // Bring window to front
                mainWindow->show();
                mainWindow->raise();
                mainWindow->activateWindow();
                
                // Move to center of screen if minimized
                if (mainWindow->windowState() == Qt::WindowMinimized) {
                    mainWindow->setWindowState(Qt::WindowNoState);
                }
                
                // Ensure it's visible on the current screen
                QScreen *currentScreen = QGuiApplication::primaryScreen();
                if (currentScreen) {
                    QRect screenGeometry = currentScreen->geometry();
                    QRect windowGeometry = mainWindow->geometry();
                    
                    // If window is outside visible area, center it
                    if (!screenGeometry.intersects(windowGeometry)) {
                        mainWindow->move(
                            screenGeometry.center().x() - windowGeometry.width() / 2,
                            screenGeometry.center().y() - windowGeometry.height() / 2
                        );
                    }
                }
                
                break;
            }
        }
    }
} 