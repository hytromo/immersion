#include "OpenAICommunicator.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QDebug>

// Constants
namespace {
    const QString DEFAULT_MODEL = "gpt-4o-mini";
    const QString API_URL = "https://api.openai.com/v1/chat/completions";
    const QString CONTENT_TYPE = "application/json";
    const QString AUTHORIZATION_PREFIX = "Bearer ";
}

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

void OpenAICommunicator::setPromptWithTemplate(const QString &promptTemplate, const QString &sourceLang, const QString &targetLang, const QString &inputText_) {
    inputText = inputText_;
    QString processedPrompt = promptTemplate;
    processedPrompt.replace("%sourceLang", sourceLang);
    processedPrompt.replace("%targetLang", targetLang);
    prompt = processedPrompt;
}

void OpenAICommunicator::setPromptRaw(const QString &prompt_) {
    prompt = prompt_;
}

QString OpenAICommunicator::getPrompt() const {
    return prompt;
}

void OpenAICommunicator::sendRequest() {
    if (apiKey.isEmpty()) {
        emit errorOccurred("API key is empty");
        return;
    }
    
    if (prompt.isEmpty()) {
        emit errorOccurred("Prompt is empty");
        return;
    }
    
    auto json = QJsonObject{};
    json["model"] = modelName.isEmpty() ? DEFAULT_MODEL : modelName;

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

    auto request = QNetworkRequest(QUrl(API_URL));
    request.setHeader(QNetworkRequest::ContentTypeHeader, CONTENT_TYPE);
    request.setRawHeader("Authorization", (AUTHORIZATION_PREFIX + apiKey).toUtf8());

    networkManager.post(request, QJsonDocument(json).toJson());
}

void OpenAICommunicator::handleNetworkReply(QNetworkReply *reply) {
    if (!reply) {
        emit errorOccurred("Invalid network reply");
        return;
    }
    
    auto responseData = reply->readAll();
    qDebug() << "API Response:" << responseData;
    
    if (reply->error() != QNetworkReply::NoError) {
        emit errorOccurred(reply->errorString() + " " + responseData);
        reply->deleteLater();
        return;
    }
    
    auto jsonDoc = QJsonDocument::fromJson(responseData);
    if (!jsonDoc.isObject()) {
        emit errorOccurred("Invalid JSON response");
        reply->deleteLater();
        return;
    }
    
    auto root = jsonDoc.object();
    auto choices = root["choices"].toArray();
    if (choices.isEmpty()) {
        emit errorOccurred("No choices returned from API");
        reply->deleteLater();
        return;
    }
    
    auto messageObj = choices[0].toObject()["message"].toObject();
    auto contentStr = messageObj["content"].toString();
    
    if (contentStr.isEmpty()) {
        emit errorOccurred("Empty content in API response");
        reply->deleteLater();
        return;
    }
    
    auto contentDoc = QJsonDocument::fromJson(contentStr.toUtf8());
    if (!contentDoc.isObject()) {
        emit errorOccurred("Failed to parse structured JSON response");
        reply->deleteLater();
        return;
    }
    
    auto result = contentDoc.object();
    auto translation = result["translation"].toString();
    
    if (translation.isEmpty()) {
        emit errorOccurred("Translation field is empty in response");
        reply->deleteLater();
        return;
    }
    
    emit replyReceived(translation);
    reply->deleteLater();
} 
