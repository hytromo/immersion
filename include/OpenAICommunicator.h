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
    void setSystemPrompt(const QString &systemPrompt);
    void setUserPrompt(const QString &userPrompt);
    void setPromptWithTemplate(const QString &promptTemplate, const QString &sourceLang, const QString &targetLang, const QString &inputText);
    
    // Request methods
    void sendRequest();
    void cancelRequest();
    
    // Getters
    QString getSystemPrompt() const;
    QString getUserPrompt() const;
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
    QString systemPrompt;
    QString userPrompt;
    QScopedPointer<QNetworkAccessManager> networkManager;
    QNetworkReply *currentReply = nullptr;
    
    // Helper methods
    bool validateRequest();
    QJsonObject createRequestPayload() const;
    QNetworkRequest createNetworkRequest() const;
    void handleApiError(const QJsonObject &errorObj);
    void handleSuccessfulResponse(const QJsonObject &responseObj);
    QString extractTranslationFromResponse(const QString &content) const;
};

#endif // OPENAICOMMUNICATOR_H 