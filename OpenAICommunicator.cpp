#include "OpenAICommunicator.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>

OpenAICommunicator::OpenAICommunicator(QObject *parent)
    : QObject(parent)
{
    connect(&networkManager, &QNetworkAccessManager::finished,
            this, &OpenAICommunicator::handleNetworkReply);
}

void OpenAICommunicator::setApiKey(const QString &key) {
    apiKey = key;
}

void OpenAICommunicator::setModelName(const QString &name) {
    modelName = name;
}

void OpenAICommunicator::setPrompt(const QString &sourceLang, const QString &targetLang, const QString &inputText_) {
    inputText = inputText_;
    prompt = QString("Translate from %1 to %2").arg(sourceLang, targetLang);
}

QString OpenAICommunicator::getPrompt() const {
    return prompt;
}

void OpenAICommunicator::sendRequest() {
    QJsonObject json;
    json["model"] = modelName.isEmpty() ? "gpt-4o-mini" : modelName;

    QJsonArray messages;
    QJsonObject message;
    message["role"] = "user";
    message["content"] = prompt;
    messages.append(message);
    messages.append(QJsonObject{
        {"role", "user"},
        {"content", inputText}
    });
    json["messages"] = messages;

    QJsonObject schema;
    schema["type"] = "object";
    schema["properties"] = QJsonObject{
        {"translation", QJsonObject{{"type", "string"}}},
    };
    schema["required"] = QJsonArray{"translation"};
    schema["additionalProperties"] = false;

    json["response_format"] = QJsonObject{
        {"type", "json_schema"},
        {"json_schema", QJsonObject{
                            {"name", "translation_response"},
                            {"strict", true},
                            {"schema", schema}
                        }}
    };

    QNetworkRequest request(QUrl("https://api.openai.com/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

    networkManager.post(request, QJsonDocument(json).toJson());
}

void OpenAICommunicator::handleNetworkReply(QNetworkReply *reply) {
    QByteArray responseData = reply->readAll();
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString());
        reply->deleteLater();
        return;
    }
    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject root = jsonDoc.object();
    QJsonArray choices = root["choices"].toArray();
    if (choices.isEmpty()) {
        emit errorOccurred("No choices returned.");
        reply->deleteLater();
        return;
    }
    QJsonObject messageObj = choices[0].toObject()["message"].toObject();
    QString contentStr = messageObj["content"].toString();
    QJsonDocument contentDoc = QJsonDocument::fromJson(contentStr.toUtf8());
    if (!contentDoc.isObject()) {
        emit errorOccurred("Failed to parse structured JSON.");
        reply->deleteLater();
        return;
    }
    QJsonObject result = contentDoc.object();
    QString translation = result["translation"].toString();
    emit replyReceived(translation);
    reply->deleteLater();
} 