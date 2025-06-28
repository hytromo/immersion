#ifndef OPENAICOMMUNICATOR_H
#define OPENAICOMMUNICATOR_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QScopedPointer>

class OpenAICommunicator : public QObject {
    Q_OBJECT
public:
    explicit OpenAICommunicator(const QString &apiKey, QObject *parent = nullptr);
    ~OpenAICommunicator() override;
    
    // Disable copy constructor and assignment operator
    OpenAICommunicator(const OpenAICommunicator &) = delete;
    OpenAICommunicator &operator=(const OpenAICommunicator &) = delete;
    
    // Configuration methods
    void setModelName(const QString &modelName);
    void setPrompt(const QString &sourceLang, const QString &targetLang, const QString &inputText);
    void setPromptWithTemplate(const QString &promptTemplate, const QString &sourceLang, const QString &targetLang, const QString &inputText);
    void setPromptRaw(const QString &prompt);
    
    // Request methods
    void sendRequest();
    void cancelRequest();
    
    // Getters
    QString getPrompt() const;
    QString getModelName() const;
    bool isRequestInProgress() const;

signals:
    void replyReceived(const QString &translation);
    void errorOccurred(const QString &errorString);
    void requestStarted();
    void requestFinished();

private slots:
    void handleNetworkReply(QNetworkReply *reply);

private:
    // Member variables
    QString apiKey;
    QString modelName;
    QString prompt;
    QString inputText;
    QScopedPointer<QNetworkAccessManager> networkManager;
    QNetworkReply *currentReply = nullptr;
    int retryCount = 0;
    
    // Helper methods
    bool validateRequest();
    QJsonObject createRequestPayload() const;
    QNetworkRequest createNetworkRequest() const;
    void handleApiError(const QJsonObject &errorObj);
    void handleSuccessfulResponse(const QJsonObject &responseObj);
    QString extractTranslationFromResponse(const QString &content) const;
    void resetRetryCount();
    void incrementRetryCount();
    bool shouldRetry() const;
    void retryRequest();
};

#endif // OPENAICOMMUNICATOR_H 