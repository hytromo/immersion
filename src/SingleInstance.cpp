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
    , m_messageCheckTimer(nullptr)
{
}

SingleInstance::~SingleInstance()
{
    // Smart pointers handle cleanup automatically
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
    m_sharedMemory.reset(new QSharedMemory(SHARED_MEMORY_KEY));
    
    if (m_sharedMemory->attach()) {
        // Another instance is already running
        m_sharedMemory->detach();
        m_sharedMemory.reset();
        return false;
    }
    
    // Create the shared memory segment
    if (!m_sharedMemory->create(SHARED_MEMORY_SIZE)) {
        // Failed to create shared memory, another instance might be running
        m_sharedMemory.reset();
        return false;
    }
    
    // Initialize the shared memory with 0
    m_sharedMemory->lock();
    memset(m_sharedMemory->data(), 0, SHARED_MEMORY_SIZE);
    m_sharedMemory->unlock();
    
    return true;
}

void SingleInstance::startListening()
{
    if (!m_sharedMemory) {
        return;
    }
    
    // Create a timer to periodically check for messages
    m_messageCheckTimer.reset(new QTimer(this));
    connect(m_messageCheckTimer.data(), &QTimer::timeout, this, &SingleInstance::checkForMessages);
    m_messageCheckTimer->start(100); // Check every 100ms
}

void SingleInstance::checkForMessages()
{
    if (!m_sharedMemory) {
        return;
    }
    
    m_sharedMemory->lock();
    int message = 0;
    memcpy(&message, m_sharedMemory->data(), sizeof(int));
    
    if (message == BRING_TO_FRONT_MESSAGE) {
        // Clear the message
        memset(m_sharedMemory->data(), 0, SHARED_MEMORY_SIZE);
        m_sharedMemory->unlock();
        
        // Emit signal to bring window to front
        emit bringToFrontRequested();
    } else {
        m_sharedMemory->unlock();
    }
}

void SingleInstance::bringExistingInstanceToFront()
{
    // Try to attach to the shared memory of the existing instance
    QSharedMemory tempSharedMemory(SHARED_MEMORY_KEY);
    
    if (tempSharedMemory.attach()) {
        // Send the bring-to-front message
        tempSharedMemory.lock();
        int message = BRING_TO_FRONT_MESSAGE;
        memcpy(tempSharedMemory.data(), &message, sizeof(int));
        tempSharedMemory.unlock();
        
        // Wait a bit for the message to be processed
        QTimer::singleShot(200, [&tempSharedMemory]() {
            tempSharedMemory.detach();
        });
        
        qDebug() << "Sent bring-to-front message to existing instance";
    } else {
        qDebug() << "Could not attach to shared memory of existing instance";
    }
} 