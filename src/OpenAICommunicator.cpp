#include "OpenAICommunicator.h"
#include "AppConfig.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QDebug>

OpenAICommunicator::OpenAICommunicator(const QString &apiKey_, QObject *parent)
    : QObject(parent)
    , apiKey(apiKey_)
    , networkManager(new QNetworkAccessManager(this))
{
    connect(networkManager.data(), &QNetworkAccessManager::finished,
            this, &OpenAICommunicator::handleNetworkReply);
}

OpenAICommunicator::~OpenAICommunicator()
{
    cancelRequest();
}

void OpenAICommunicator::setModelName(const QString &name) {
    modelName = name;
}

void OpenAICommunicator::setSystemPrompt(const QString &systemPrompt_) {
    systemPrompt = systemPrompt_;
}

void OpenAICommunicator::setUserPrompt(const QString &userPrompt_) {
    userPrompt = userPrompt_;
}

QString OpenAICommunicator::getSystemPrompt() const {
    return systemPrompt;
}

QString OpenAICommunicator::getUserPrompt() const {
    return userPrompt;
}

bool OpenAICommunicator::isRequestInProgress() const {
    return currentReply != nullptr && currentReply->isRunning();
}

bool OpenAICommunicator::validateRequest() {
    if (apiKey.isEmpty()) {
        qDebug() << "[OpenAICommunicator] API key is empty";
        emit errorOccurred("API key is empty");
        return false;
    }
    if (systemPrompt.isEmpty()) {
        qDebug() << "[OpenAICommunicator] System prompt is empty";
        emit errorOccurred("System prompt is empty");
        return false;
    }
    if (userPrompt.isEmpty()) {
        qDebug() << "[OpenAICommunicator] User prompt is empty";
        emit errorOccurred("User prompt is empty");
        return false;
    }
    qDebug() << "[OpenAICommunicator] validateRequest passed";
    return true;
}

QJsonObject OpenAICommunicator::createRequestPayload() const {
    auto json = QJsonObject{};
    json["model"] = getModelName();

    auto messages = QJsonArray{};
    // Add the system prompt if it exists
    if (!systemPrompt.isEmpty()) {
        auto systemMessage = QJsonObject{};
        systemMessage["role"] = "system";
        systemMessage["content"] = systemPrompt;
        messages.append(systemMessage);
    }
    // Add the user message with user prompt
    auto userMessage = QJsonObject{};
    userMessage["role"] = "user";
    userMessage["content"] = userPrompt;
    messages.append(userMessage);
    json["messages"] = messages;

    return json;
}

QNetworkRequest OpenAICommunicator::createNetworkRequest() const {
    auto request = QNetworkRequest(QUrl(AppConfig::OPENAI_API_URL));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + apiKey).toUtf8());
    request.setTransferTimeout(AppConfig::API_REQUEST_TIMEOUT_MS);
    request.setRawHeader("Accept", "application/json");
    return request;
}

void OpenAICommunicator::sendRequest() {
    qDebug() << "[OpenAICommunicator] sendRequest called";
    if (!validateRequest()) {
        qDebug() << "[OpenAICommunicator] validateRequest failed";
        return;
    }
    
    // Cancel any existing request
    if (currentReply && currentReply->isRunning()) {
        currentReply->abort();
    }
    currentReply = nullptr;
    
    const auto json = createRequestPayload();
    const auto request = createNetworkRequest();

    qDebug() << "[OpenAICommunicator] Payload:" << QJsonDocument(json).toJson();

    qDebug() << "[OpenAICommunicator] Posting network request to" << request.url();
    currentReply = networkManager->post(request, QJsonDocument(json).toJson());
    
    if (!currentReply) {
        qDebug() << "[OpenAICommunicator] Failed to create network request";
        emit errorOccurred("Failed to create network request");
        return;
    }
    
    emit requestStarted();
}

void OpenAICommunicator::cancelRequest() {
    if (currentReply && currentReply->isRunning()) {
        currentReply->abort();
    }
    currentReply = nullptr;
}

void OpenAICommunicator::handleNetworkReply(QNetworkReply *reply) {
    qDebug() << "[OpenAICommunicator] handleNetworkReply called";
    if (!reply) {
        qDebug() << "[OpenAICommunicator] Invalid network reply";
        emit errorOccurred("Invalid network reply");
        emit requestFinished();
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
        emit requestFinished();
        return;
    }
    
    const auto jsonDoc = QJsonDocument::fromJson(responseData);
    if (!jsonDoc.isObject()) {
        emit errorOccurred("Invalid JSON response");
        emit requestFinished();
        return;
    }
    
    const auto root = jsonDoc.object();
    
    // Check for API errors
    if (root.contains("error")) {
        handleApiError(root["error"].toObject());
        emit requestFinished();
        return;
    }
    
    handleSuccessfulResponse(root);
    emit requestFinished();
}

void OpenAICommunicator::handleApiError(const QJsonObject &errorObj) {
    qDebug() << "handleApiError called with:" << QJsonDocument(errorObj).toJson();
    
    const QString errorMessage = errorObj["message"].toString();
    const QString errorType = errorObj["type"].toString();
    
    QString fullErrorMessage = "API Error";
    if (!errorType.isEmpty()) {
        fullErrorMessage += " (" + errorType + ")";
    }
    fullErrorMessage += ": " + errorMessage;
    
    emit errorOccurred(fullErrorMessage);
    emit requestFinished();
}

void OpenAICommunicator::handleSuccessfulResponse(const QJsonObject &responseObj) {
    const auto choices = responseObj["choices"].toArray();
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

    emit replyReceived(contentStr);
}

QString OpenAICommunicator::extractTranslationFromResponse(const QString &content) const {
    // Try to parse as structured JSON first
    const auto contentDoc = QJsonDocument::fromJson(content.toUtf8());
    if (contentDoc.isObject()) {
        const auto result = contentDoc.object();
        const auto translation = result["translation"].toString();
        
        if (!translation.isEmpty()) {
            return translation;
        }
    }
    
    // If structured parsing fails, treat the entire content as the translation
    // This handles cases where the API doesn't follow the structured format
    return content;
}

QString OpenAICommunicator::getModelName() const {
    return modelName.isEmpty() ? AppConfig::DEFAULT_OPENAI_MODEL : modelName;
}
