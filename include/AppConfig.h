#ifndef APPCONFIG_H
#define APPCONFIG_H

#include <QString>

class AppConfig
{
public:
    // Application metadata
    static const QString ORGANIZATION_NAME;
    static const QString ORGANIZATION_DOMAIN;
    static const QString APPLICATION_NAME;
    static const QString APPLICATION_VERSION;
    
    // API Configuration
    static const QString DEFAULT_OPENAI_MODEL;
    static const QString OPENAI_API_URL;
    static const int API_REQUEST_TIMEOUT_MS;
    static const int API_MAX_RETRIES;
    
    // UI Configuration
    static const int MAX_HISTORY_SIZE;
    static const int MAX_HISTORY_DISPLAY_LENGTH;
    static const int SPELL_CHECK_DELAY_MS;
    
    // File Configuration
    static const QString LOG_FILE_EXTENSION;
    static const QString REPORT_FILE_EXTENSION;
    
    // Keychain Configuration
    static const QString OPENAI_API_KEY_KEYCHAIN_KEY;
    
    // Network Configuration
    static const int NETWORK_RETRY_DELAY_MS;
    
private:
    // Private constructor to prevent instantiation
    AppConfig() = delete;
};

#endif // APPCONFIG_H 