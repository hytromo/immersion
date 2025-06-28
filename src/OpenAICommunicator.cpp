#include "OpenAICommunicator.h"
#include "AppConfig.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkRequest>
#include <QDebug>
#include <QTimer>

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

QString OpenAICommunicator::getModelName() const {
    return modelName.isEmpty() ? AppConfig::DEFAULT_OPENAI_MODEL : modelName;
}

bool OpenAICommunicator::isRequestInProgress() const {
    return currentReply != nullptr && currentReply->isRunning();
}

bool OpenAICommunicator::validateRequest() {
    if (apiKey.isEmpty()) {
        emit errorOccurred("API key is empty");
        return false;
    }
    
    if (prompt.isEmpty()) {
        emit errorOccurred("Prompt is empty");
        return false;
    }
    
    if (inputText.isEmpty()) {
        emit errorOccurred("Input text is empty");
        return false;
    }
    
    return true;
}

QJsonObject OpenAICommunicator::createRequestPayload() const {
    auto json = QJsonObject{};
    json["model"] = getModelName();

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
        {"json_schema", QJsonObject{
            {"name", "translation_response"},
            {"schema", schema}
        }}
    };

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
    if (!validateRequest()) {
        return;
    }
    
    // Cancel any existing request but preserve retry count
    if (currentReply && currentReply->isRunning()) {
        currentReply->abort();
    }
    currentReply = nullptr;
    
    // Only reset retry count for new requests, not retries
    if (retryCount == 0) {
        resetRetryCount();
    }
    
    const auto json = createRequestPayload();
    const auto request = createNetworkRequest();

    currentReply = networkManager->post(request, QJsonDocument(json).toJson());
    
    if (!currentReply) {
        emit errorOccurred("Failed to create network request");
        return;
    }
    
    emit requestStarted();
    
    // Connect to the reply's signals for better error handling
    connect(currentReply, &QNetworkReply::errorOccurred, this, [this](QNetworkReply::NetworkError error) {
        if (currentReply && currentReply->isRunning()) {
            QString errorMessage = currentReply->errorString();
            if (error == QNetworkReply::OperationCanceledError) {
                errorMessage = "Request was cancelled";
            } else if (error == QNetworkReply::TimeoutError) {
                errorMessage = "Request timed out";
            }
            
            if (shouldRetry()) {
                incrementRetryCount();
                QTimer::singleShot(AppConfig::NETWORK_RETRY_DELAY_MS * retryCount, this, &OpenAICommunicator::retryRequest);
            } else {
                emit errorOccurred(errorMessage);
                emit requestFinished();
            }
        }
    });
}

void OpenAICommunicator::cancelRequest() {
    if (currentReply && currentReply->isRunning()) {
        currentReply->abort();
    }
    currentReply = nullptr;
    resetRetryCount();
}

void OpenAICommunicator::resetRetryCount() {
    retryCount = 0;
}

void OpenAICommunicator::incrementRetryCount() {
    retryCount++;
}

bool OpenAICommunicator::shouldRetry() const {
    return retryCount < AppConfig::API_MAX_RETRIES;
}

void OpenAICommunicator::retryRequest() {
    qDebug() << "retryRequest called - retryCount:" << retryCount << "shouldRetry:" << shouldRetry() << "isRequestInProgress:" << isRequestInProgress();
    if (shouldRetry() && !isRequestInProgress()) {
        qDebug() << "Retrying request, attempt" << (retryCount + 1) << "of" << AppConfig::API_MAX_RETRIES;
        sendRequest();
    } else {
        qDebug() << "Not retrying - shouldRetry:" << shouldRetry() << "isRequestInProgress:" << isRequestInProgress();
    }
}

void OpenAICommunicator::handleNetworkReply(QNetworkReply *reply) {
    if (!reply) {
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
    
    // Check if reply is still open before reading
    if (!reply->isOpen()) {
        QString errorMessage = "Network reply is not open";
        if (shouldRetry()) {
            incrementRetryCount();
            QTimer::singleShot(AppConfig::NETWORK_RETRY_DELAY_MS * retryCount, this, &OpenAICommunicator::retryRequest);
        } else {
            emit errorOccurred(errorMessage);
            emit requestFinished();
        }
        return;
    }
    
    const auto responseData = reply->readAll();
    qDebug() << "API Response:" << responseData;
    
    if (reply->error() != QNetworkReply::NoError) {
        QString errorMessage = reply->errorString();
        if (reply->error() == QNetworkReply::OperationCanceledError) {
            errorMessage = "Request was cancelled";
        } else if (reply->error() == QNetworkReply::TimeoutError) {
            errorMessage = "Request timed out";
        }
        
        // Don't retry for certain types of errors
        bool shouldRetryError = shouldRetry() && 
                               reply->error() != QNetworkReply::OperationCanceledError &&
                               reply->error() != QNetworkReply::ContentAccessDenied &&
                               reply->error() != QNetworkReply::ContentOperationNotPermittedError;
        
        if (shouldRetryError) {
            incrementRetryCount();
            QTimer::singleShot(AppConfig::NETWORK_RETRY_DELAY_MS * retryCount, this, &OpenAICommunicator::retryRequest);
        } else {
            emit errorOccurred(errorMessage + " " + responseData);
            emit requestFinished();
        }
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
    const QString errorCode = errorObj["code"].toString();
    
    QString fullErrorMessage = "API Error";
    if (!errorType.isEmpty()) {
        fullErrorMessage += " (" + errorType + ")";
    }
    fullErrorMessage += ": " + errorMessage;
    
    // Don't retry for parameter errors as they won't be fixed by retrying
    bool isParameterError = errorCode == "missing_required_parameter" || 
                           errorCode == "invalid_parameter" ||
                           errorType == "invalid_request_error";
    
    qDebug() << "API Error - retryCount:" << retryCount << "shouldRetry:" << shouldRetry() << "isParameterError:" << isParameterError;
    
    // Always stop retrying for parameter errors or when max retries reached
    if (isParameterError || !shouldRetry()) {
        qDebug() << "Stopping retries - parameter error or max retries reached";
        emit errorOccurred(fullErrorMessage);
        emit requestFinished();
    } else {
        qDebug() << "Retrying API error";
        incrementRetryCount();
        QTimer::singleShot(AppConfig::NETWORK_RETRY_DELAY_MS * retryCount, this, &OpenAICommunicator::retryRequest);
    }
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
    
    const QString translation = extractTranslationFromResponse(contentStr);
    if (!translation.isEmpty()) {
        emit replyReceived(translation);
    } else {
        emit errorOccurred("Failed to extract translation from response");
    }
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
