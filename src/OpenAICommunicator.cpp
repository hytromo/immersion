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
    const int REQUEST_TIMEOUT_MS = 30000; // 30 seconds
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
    
    if (inputText.isEmpty()) {
        emit errorOccurred("Input text is empty");
        return;
    }
    
    // Cancel any existing request
    cancelRequest();
    
    auto json = QJsonObject{};
    json["model"] = modelName.isEmpty() ? DEFAULT_MODEL : modelName;

    auto messages = QJsonArray{};
    
    // Add the system prompt if it exists
    if (!prompt.isEmpty()) {
        auto systemMessage = QJsonObject{};
        systemMessage["role"] = "system";
        systemMessage["content"] = prompt;
        messages.append(systemMessage);
    }
    
    // Add the user message with input text
    auto userMessage = QJsonObject{};
    userMessage["role"] = "user";
    userMessage["content"] = inputText;
    messages.append(userMessage);
    
    json["messages"] = messages;

    // Add response format for structured output
    auto schema = QJsonObject{};
    schema["type"] = "object";
    schema["properties"] = QJsonObject{
        {"translation", QJsonObject{{"type", "string"}}},
    };
    schema["required"] = QJsonArray{"translation"};
    schema["additionalProperties"] = false;

    json["response_format"] = QJsonObject{
        {"type", "json_schema"},
        {"json_schema", schema}
    };

    auto request = QNetworkRequest(QUrl(API_URL));
    request.setHeader(QNetworkRequest::ContentTypeHeader, CONTENT_TYPE);
    request.setRawHeader("Authorization", (AUTHORIZATION_PREFIX + apiKey).toUtf8());
    
    // Set timeout
    request.setTransferTimeout(REQUEST_TIMEOUT_MS);

    currentReply = networkManager.post(request, QJsonDocument(json).toJson());
    
    if (!currentReply) {
        emit errorOccurred("Failed to create network request");
        return;
    }
    
    // Connect to the reply's signals for better error handling
    connect(currentReply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError error) {
        if (currentReply) {
            QString errorMessage = currentReply->errorString();
            if (error == QNetworkReply::OperationCanceledError) {
                errorMessage = "Request was cancelled";
            } else if (error == QNetworkReply::TimeoutError) {
                errorMessage = "Request timed out";
            }
            emit errorOccurred(errorMessage);
        }
    });
}

void OpenAICommunicator::cancelRequest() {
    if (currentReply && currentReply->isRunning()) {
        currentReply->abort();
    }
    currentReply = nullptr;
}

void OpenAICommunicator::handleNetworkReply(QNetworkReply *reply) {
    if (!reply) {
        emit errorOccurred("Invalid network reply");
        return;
    }
    
    // Clear the current reply reference
    if (reply == currentReply) {
        currentReply = nullptr;
    }
    
    // Ensure reply is deleted when we exit this function
    const auto replyDeleter = qScopeGuard([reply]() {
        reply->deleteLater();
    });
    
    const auto responseData = reply->readAll();
    qDebug() << "API Response:" << responseData;
    
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMessage = reply->errorString();
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            errorMessage = "Request was cancelled";
        } else if (reply->error() == QNetworkReply::TimeoutError) {
            errorMessage = "Request timed out";
        }
        emit errorOccurred(errorMessage + " " + responseData);
        return;
    }
    
    const auto jsonDoc = QJsonDocument::fromJson(responseData);
    if (!jsonDoc.isObject()) {
        emit errorOccurred("Invalid JSON response");
        return;
    }
    
    const auto root = jsonDoc.object();
    
    // Check for API errors
    if (root.contains("error")) {
        const auto errorObj = root["error"].toObject();
        const QString errorMessage = errorObj["message"].toString();
        emit errorOccurred("API Error: " + errorMessage);
        return;
    }
    
    const auto choices = root["choices"].toArray();
    if (choices.isEmpty()) {
        emit errorOccurred("No choices returned from API");
        return;
    }
    
    const auto messageObj = choices[0].toObject()["message"].toObject();
    const auto contentStr = messageObj["content"].toString();
    
    if (contentStr.isEmpty()) {
        emit errorOccurred("Empty content in API response");
        return;
    }
    
    // Try to parse as structured JSON first
    const auto contentDoc = QJsonDocument::fromJson(contentStr.toUtf8());
    if (contentDoc.isObject()) {
        const auto result = contentDoc.object();
        const auto translation = result["translation"].toString();
        
        if (!translation.isEmpty()) {
            emit replyReceived(translation);
            return;
        }
    }
    
    // If structured parsing fails, treat the entire content as the translation
    // This handles cases where the API doesn't follow the structured format
    emit replyReceived(contentStr);
} 
