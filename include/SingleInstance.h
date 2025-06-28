#ifndef SINGLEINSTANCE_H
#define SINGLEINSTANCE_H

#include <QObject>
#include <QSharedMemory>
#include <QTimer>
#include <QWindow>

class SingleInstance : public QObject
{
    Q_OBJECT

public:
    explicit SingleInstance(QObject *parent = nullptr);
    ~SingleInstance();

    bool isAnotherInstanceRunning();
    bool tryToRun();
    void bringExistingInstanceToFront();

private:
    QSharedMemory *m_sharedMemory;
    static const QString SHARED_MEMORY_KEY;
    static const int SHARED_MEMORY_SIZE = 1;
};

#endif // SINGLEINSTANCE_H 