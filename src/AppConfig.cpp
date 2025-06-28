#include "AppConfig.h"

// Application metadata
const QString AppConfig::ORGANIZATION_NAME = "hytromo";
const QString AppConfig::ORGANIZATION_DOMAIN = "hytromo.github.io";
const QString AppConfig::APPLICATION_NAME = "immersion";
const QString AppConfig::APPLICATION_VERSION = "0.1";

// API Configuration
const QString AppConfig::DEFAULT_OPENAI_MODEL = "gpt-4o-mini";
const QString AppConfig::OPENAI_API_URL = "https://api.openai.com/v1/chat/completions";
const int AppConfig::API_REQUEST_TIMEOUT_MS = 30000; // 30 seconds
const int AppConfig::API_MAX_RETRIES = 3;

// UI Configuration
const int AppConfig::MAX_HISTORY_SIZE = 5;
const int AppConfig::MAX_HISTORY_DISPLAY_LENGTH = 50;
const int AppConfig::SPELL_CHECK_DELAY_MS = 500;

// File Configuration
const QString AppConfig::LOG_FILE_EXTENSION = ".txt";
const QString AppConfig::REPORT_FILE_EXTENSION = "_report.txt";

// Keychain Configuration
const QString AppConfig::OPENAI_API_KEY_KEYCHAIN_KEY = "hytromo/immersion/openai_api_key";

// Network Configuration
const int AppConfig::NETWORK_RETRY_DELAY_MS = 1000; // 1 second 