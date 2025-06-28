#ifndef NETWORKREQUESTMANAGER_H
#define NETWORKREQUESTMANAGER_H

#include "OpenAICommunicator.h"
#include <QDialog>
#include <QObject>
#include <functional>

class NetworkRequestManager : public QObject
{
    Q_OBJECT

public:
    explicit NetworkRequestManager(QObject *parent = nullptr);

    // Generic method to handle OpenAI requests with progress dialog
    void executeRequest(const QString &apiKey, 
                       const QString &modelName,
                       const QString &prompt,
                       const QString &inputText,
                       QWidget *parent,
                       const QString &progressTitle,
                       std::function<void(const QString&)> successCallback,
                       std::function<void(const QString&)> errorCallback = nullptr);

    // Method to handle report generation requests
    void executeReportRequest(const QString &apiKey,
                             const QString &modelName,
                             const QString &prompt,
                             const QString &fileContent,
                             QWidget *parent,
                             std::function<void(const QString&)> successCallback,
                             std::function<void(const QString&)> errorCallback = nullptr);

private:
    void cleanupProgressAndCommunicator(QDialog *progress, OpenAICommunicator *communicator, QWidget *parentWidget);
    void connectOpenAICommunicator(OpenAICommunicator *communicator, 
                                  QDialog *progress,
                                  QWidget *parentWidget,
                                  std::function<void(const QString&)> successCallback,
                                  std::function<void(const QString&)> errorCallback);
};

#endif // NETWORKREQUESTMANAGER_H 