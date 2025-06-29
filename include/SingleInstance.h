#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H

#include <QObject>
#include <QSharedMemory>
#include <QTimer>
#include <QWindow>
#include <QScopedPointer>

class SingleInstance : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstance(QObject *parent = nullptr);
    ~SingleInstance();

    bool isAnotherInstanceRunning();
    bool tryToRun();
    void bringExistingInstanceToFront();
    void startListening();

signals:
    void bringToFrontRequested();

private slots:
    void checkForMessages();

private:
    QScopedPointer<QSharedMemory> m_sharedMemory;
    QScopedPointer<QTimer> m_messageCheckTimer;
    static const QString SHARED_MEMORY_KEY;
    static const int SHARED_MEMORY_SIZE = 4; // Enough for a simple message
    static const int BRING_TO_FRONT_MESSAGE = 1;
};

#endif // SINGLEINSTANCE_H 