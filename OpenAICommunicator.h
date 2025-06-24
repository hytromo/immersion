#ifndef OPENAICOMMUNICATOR_H
#define OPENAICOMMUNICATOR_H

#include <QObject>
#include <QString>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

class OpenAICommunicator : public QObject {
    Q_OBJECT
public:
    explicit OpenAICommunicator(QObject *parent = nullptr);
    void setApiKey(const QString &apiKey);
    void setModelName(const QString &modelName);
    void setPrompt(const QString &sourceLang, const QString &targetLang, const QString &inputText);
    void sendRequest();
    QString getPrompt() const;

signals:
    void replyReceived(const QString &translation);
    void errorOccurred(const QString &errorString);

private slots:
    void handleNetworkReply(QNetworkReply *reply);

private:
    QString apiKey;
    QString modelName;
    QString prompt;
    QString inputText;
    QNetworkAccessManager networkManager;
};

#endif // OPENAICOMMUNICATOR_H 