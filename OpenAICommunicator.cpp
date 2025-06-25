#include "OpenAICommunicator.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>

OpenAICommunicator::OpenAICommunicator(const QString &apiKey_, QObject *parent)
    : QObject(parent), apiKey(apiKey_)
{
    connect(&networkManager, &QNetworkAccessManager::finished,
            this, &OpenAICommunicator::handleNetworkReply);
}

void OpenAICommunicator::setModelName(const QString &name) {
    modelName = name;
}

void OpenAICommunicator::setPrompt(const QString &sourceLang, const QString &targetLang, const QString &inputText_) {
    inputText = inputText_;
    prompt = QString("Translate from %1 to %2").arg(sourceLang, targetLang);
}

void OpenAICommunicator::setPromptRaw(const QString &prompt_) {
    prompt = prompt_;
}

QString OpenAICommunicator::getPrompt() const {
    return prompt;
}

void OpenAICommunicator::sendRequest() {
    auto json = QJsonObject{};
    json["model"] = modelName.isEmpty() ? "gpt-4o-mini" : modelName;

    auto messages = QJsonArray{};
    auto message = QJsonObject{};
    message["role"] = "user";
    message["content"] = prompt;
    messages.append(message);
    messages.append(QJsonObject{
        {"role", "user"},
        {"content", inputText}
    });
    json["messages"] = messages;

    auto schema = QJsonObject{};
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

    auto request = QNetworkRequest(QUrl("https://api.openai.com/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());

    networkManager.post(request, QJsonDocument(json).toJson());
}

void OpenAICommunicator::handleNetworkReply(QNetworkReply *reply) {
    auto responseData = reply->readAll();
    qDebug() << responseData;
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString() + " " + responseData);
        reply->deleteLater();
        return;
    }
    auto jsonDoc = QJsonDocument::fromJson(responseData);
    auto root = jsonDoc.object();
    auto choices = root["choices"].toArray();
    if (choices.isEmpty()) {
        emit errorOccurred("No choices returned.");
        reply->deleteLater();
        return;
    }
    auto messageObj = choices[0].toObject()["message"].toObject();
    auto contentStr = messageObj["content"].toString();
    auto contentDoc = QJsonDocument::fromJson(contentStr.toUtf8());
    if (!contentDoc.isObject()) {
        emit errorOccurred("Failed to parse structured JSON.");
        reply->deleteLater();
        return;
    }
    auto result = contentDoc.object();
    auto translation = result["translation"].toString();
    emit replyReceived(translation);
    reply->deleteLater();
} 
