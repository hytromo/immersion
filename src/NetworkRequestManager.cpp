#include "NetworkRequestManager.h"
#include "ProgressDialog.h"
#include <QMessageBox>
#include <memory>

NetworkRequestManager::NetworkRequestManager(QObject *parent)
    : QObject(parent)
{
}

void NetworkRequestManager::executeRequest(const QString &apiKey, 
                                         const QString &modelName,
                                         const QString &prompt,
                                         const QString &inputText,
                                         QWidget *parent,
                                         const QString &progressTitle,
                                         std::function<void(const QString&)> successCallback,
                                         std::function<void(const QString&)> errorCallback)
{
    if (apiKey.isEmpty()) {
        if (errorCallback) {
            errorCallback("API key is missing");
        } else {
            QMessageBox::warning(parent, "Error", "OpenAI API key is missing.");
        }
        return;
    }

    parent->setEnabled(false);
    
    // Using std::unique_ptr for better memory management
    std::unique_ptr<ProgressDialog> progress(new ProgressDialog(parent));
    std::unique_ptr<OpenAICommunicator> openaiCommunicator(new OpenAICommunicator(apiKey, this));
    
    if (!progressTitle.isEmpty()) {
        progress->setWindowTitle(progressTitle);
    }
    progress->show();

    openaiCommunicator->setModelName(modelName);
    openaiCommunicator->setPromptRaw(prompt + "\n\n" + inputText);
    openaiCommunicator->sendRequest();

    // Transfer ownership to the connection function using release()
    connectOpenAICommunicator(openaiCommunicator.release(), progress.release(), parent, successCallback, errorCallback);
}

void NetworkRequestManager::executeReportRequest(const QString &apiKey,
                                               const QString &modelName,
                                               const QString &prompt,
                                               const QString &fileContent,
                                               QWidget *parent,
                                               std::function<void(const QString&)> successCallback,
                                               std::function<void(const QString&)> errorCallback)
{
    if (apiKey.isEmpty()) {
        if (errorCallback) {
            errorCallback("API key is missing");
        } else {
            QMessageBox::warning(parent, "Error", "OpenAI API key is missing.");
        }
        return;
    }

    if (fileContent.isEmpty()) {
        if (errorCallback) {
            errorCallback("File content is empty");
        } else {
            QMessageBox::warning(parent, "Error", "Could not open file.");
        }
        return;
    }

    parent->setEnabled(false);
    
    // Using std::unique_ptr for better memory management
    std::unique_ptr<ProgressDialog> progress(new ProgressDialog(parent));
    std::unique_ptr<OpenAICommunicator> openaiCommunicator(new OpenAICommunicator(apiKey, this));
    progress->show();

    openaiCommunicator->setModelName(modelName);
    openaiCommunicator->setPromptRaw(prompt + "\n\n" + fileContent);
    openaiCommunicator->sendRequest();

    // Transfer ownership to the connection function using release()
    connectOpenAICommunicator(openaiCommunicator.release(), progress.release(), parent, successCallback, errorCallback);
}

void NetworkRequestManager::cleanupProgressAndCommunicator(QDialog *progress, OpenAICommunicator *communicator, QWidget *parentWidget)
{
    if (progress) {
        progress->close();
        progress->deleteLater();
    }
    if (communicator) {
        communicator->deleteLater();
    }
    
    // Re-enable the parent widget
    if (parentWidget) {
        parentWidget->setEnabled(true);
    }
}

void NetworkRequestManager::connectOpenAICommunicator(OpenAICommunicator *communicator, 
                                                    QDialog *progress,
                                                    QWidget *parent,
                                                    std::function<void(const QString&)> successCallback,
                                                    std::function<void(const QString&)> errorCallback)
{
    connect(communicator, &OpenAICommunicator::replyReceived, this, 
            [=](const QString &response) mutable {
                cleanupProgressAndCommunicator(progress, communicator, parent);
                if (successCallback) {
                    successCallback(response);
                }
            });
    
    connect(communicator, &OpenAICommunicator::errorOccurred, this, 
            [=](const QString &errorString) mutable {
                cleanupProgressAndCommunicator(progress, communicator, parent);
                if (errorCallback) {
                    errorCallback(errorString);
                } else {
                    QMessageBox::warning(parent, "Network Error", errorString);
                }
            });
} 