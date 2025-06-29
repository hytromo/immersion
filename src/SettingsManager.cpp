#include "SettingsManager.h"

// Settings Keys
namespace {
    const QString SETTINGS_TRANSLATION_MODEL_NAME_KEY = "translation_model_name";
    const QString SETTINGS_REPORT_MODEL_NAME_KEY = "report_model_name";
    const QString SETTINGS_FEEDBACK_MODEL_NAME_KEY = "feedback_model_name";
    const QString SETTINGS_SOURCE_LANG_KEY = "sourceLang";
    const QString SETTINGS_TARGET_LANG_KEY = "targetLang";
    const QString SETTINGS_LAST_INPUT_KEY = "lastInputText";
    const QString SETTINGS_TRANSLATION_PROMPT_KEY = "translation_prompt";
    const QString SETTINGS_REPORT_PROMPT_KEY = "report_prompt";
    const QString SETTINGS_FEEDBACK_PROMPT_KEY = "feedback_prompt";
    const QString SETTINGS_MESSAGE_HISTORY_KEY = "message_history";
    const QString SETTINGS_SPELLCHECKER_LANGUAGE_KEY = "spellCheckerLanguage";
    const QString SETTINGS_VISUAL_SPELLCHECKING_KEY = "visualSpellCheckingEnabled";
    const QString SETTINGS_QUICK_FEEDBACK_KEY = "quickFeedbackEnabled";
    
    // Default Values
    const QString DEFAULT_TRANSLATION_MODEL = "gpt-4o-mini";
    const QString DEFAULT_REPORT_MODEL = "gpt-4o-mini";
    const QString DEFAULT_FEEDBACK_MODEL = "gpt-4o-mini";
    const QString DEFAULT_SOURCE_LANG = "Danish";
    const QString DEFAULT_TARGET_LANG = "English";
    const QString DEFAULT_SPELLCHECKER_LANG = "en_US";
    const int MAX_HISTORY_SIZE = 10;
    
    // Default Prompts
    const QString DEFAULT_TRANSLATION_PROMPT = "You are an expert %sourceLang to %targetLang translator. Translate this text making sure to match the tone and style of the original.";
    const QString DEFAULT_REPORT_PROMPT = "You are an expert %sourceLang teacher. Find the top 5 grammatical mistakes in this %sourceLang text. Format your response as:\n\nORIGINAL: [sentence with mistakes]\nCORRECTED: [fully corrected sentence]\nEXPLANATIONS: [list of explanations for each mistake, one explanation per line, starting with dash]\n\nSeparate entries with two empty lines. If fewer than 5 grammatical errors exist, include important spelling mistakes. Keep explanations in English.";
    const QString DEFAULT_FEEDBACK_PROMPT = "You are an expert %sourceLang teacher. Provide feedback in English on the syntax, grammar, and fluency of this %sourceLang text. Be constructive and specific. Format your response as:\n\nSYNTAX: [feedback on sentence structure]\nGRAMMAR: [feedback on grammatical correctness]\nFLUENCY: [feedback on naturalness and flow]\n\nKeep each section concise but helpful.";
}

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent), settings(parent)
{
}

// Model Name Getters and Setters
QString SettingsManager::translationModelName() const {
    return settings.value(SETTINGS_TRANSLATION_MODEL_NAME_KEY, DEFAULT_TRANSLATION_MODEL).toString();
}

void SettingsManager::setTranslationModelName(const QString &name) {
    if (!name.trimmed().isEmpty()) {
        settings.setValue(SETTINGS_TRANSLATION_MODEL_NAME_KEY, name.trimmed());
    }
}

QString SettingsManager::reportModelName() const {
    return settings.value(SETTINGS_REPORT_MODEL_NAME_KEY, DEFAULT_REPORT_MODEL).toString();
}

void SettingsManager::setReportModelName(const QString &name) {
    if (!name.trimmed().isEmpty()) {
        settings.setValue(SETTINGS_REPORT_MODEL_NAME_KEY, name.trimmed());
    }
}

QString SettingsManager::feedbackModelName() const {
    return settings.value(SETTINGS_FEEDBACK_MODEL_NAME_KEY, DEFAULT_FEEDBACK_MODEL).toString();
}

void SettingsManager::setFeedbackModelName(const QString &name) {
    if (!name.trimmed().isEmpty()) {
        settings.setValue(SETTINGS_FEEDBACK_MODEL_NAME_KEY, name.trimmed());
    }
}

// Language Getters and Setters
QString SettingsManager::sourceLang() const {
    return settings.value(SETTINGS_SOURCE_LANG_KEY, DEFAULT_SOURCE_LANG).toString();
}

void SettingsManager::setSourceLang(const QString &lang) {
    if (!lang.trimmed().isEmpty()) {
        settings.setValue(SETTINGS_SOURCE_LANG_KEY, lang.trimmed());
    }
}

QString SettingsManager::targetLang() const {
    return settings.value(SETTINGS_TARGET_LANG_KEY, DEFAULT_TARGET_LANG).toString();
}

void SettingsManager::setTargetLang(const QString &lang) {
    if (!lang.trimmed().isEmpty()) {
        settings.setValue(SETTINGS_TARGET_LANG_KEY, lang.trimmed());
    }
}

// Input Text Getter and Setter
QString SettingsManager::lastInputText() const {
    return settings.value(SETTINGS_LAST_INPUT_KEY, "").toString();
}

void SettingsManager::setLastInputText(const QString &text) {
    settings.setValue(SETTINGS_LAST_INPUT_KEY, text);
}

// Prompt Getters and Setters
QString SettingsManager::translationPrompt() const {
    return settings.value(SETTINGS_TRANSLATION_PROMPT_KEY, DEFAULT_TRANSLATION_PROMPT).toString();
}

void SettingsManager::setTranslationPrompt(const QString &prompt) {
    if (!prompt.trimmed().isEmpty()) {
        settings.setValue(SETTINGS_TRANSLATION_PROMPT_KEY, prompt.trimmed());
    }
}

QString SettingsManager::reportPrompt() const {
    return settings.value(SETTINGS_REPORT_PROMPT_KEY, DEFAULT_REPORT_PROMPT).toString();
}

void SettingsManager::setReportPrompt(const QString &prompt) {
    if (!prompt.trimmed().isEmpty()) {
        settings.setValue(SETTINGS_REPORT_PROMPT_KEY, prompt.trimmed());
    }
}

QString SettingsManager::feedbackPrompt() const {
    return settings.value(SETTINGS_FEEDBACK_PROMPT_KEY, DEFAULT_FEEDBACK_PROMPT).toString();
}

void SettingsManager::setFeedbackPrompt(const QString &prompt) {
    if (!prompt.trimmed().isEmpty()) {
        settings.setValue(SETTINGS_FEEDBACK_PROMPT_KEY, prompt.trimmed());
    }
}

// Default Prompt Getters
QString SettingsManager::getDefaultTranslationPrompt() const {
    return DEFAULT_TRANSLATION_PROMPT;
}

QString SettingsManager::getDefaultReportPrompt() const {
    return DEFAULT_REPORT_PROMPT;
}

QString SettingsManager::getDefaultFeedbackPrompt() const {
    return DEFAULT_FEEDBACK_PROMPT;
}

// Message History Management
QStringList SettingsManager::getMessageHistory() const {
    QVariant historyVariant = settings.value(SETTINGS_MESSAGE_HISTORY_KEY);
    if (historyVariant.canConvert<QStringList>()) {
        return historyVariant.toStringList();
    }
    return QStringList();
}

void SettingsManager::addMessageToHistory(const QString &message) {
    QString trimmedMessage = message.trimmed();
    if (trimmedMessage.isEmpty()) {
        return; // Don't add empty messages
    }
    
    QStringList history = getMessageHistory();
    
    // Remove the message if it already exists (to avoid duplicates)
    history.removeAll(trimmedMessage);
    
    // Add the new message at the beginning
    history.prepend(trimmedMessage);
    
    // Keep only the last MAX_HISTORY_SIZE messages
    if (history.size() > MAX_HISTORY_SIZE) {
        history = history.mid(0, MAX_HISTORY_SIZE);
    }
    
    settings.setValue(SETTINGS_MESSAGE_HISTORY_KEY, QVariant(history));
}

void SettingsManager::clearMessageHistory() {
    settings.remove(SETTINGS_MESSAGE_HISTORY_KEY);
}

// Settings Synchronization
void SettingsManager::sync() {
    settings.sync();
}

// Spell Checker Settings
QString SettingsManager::spellCheckerLanguage() const {
    return settings.value(SETTINGS_SPELLCHECKER_LANGUAGE_KEY, DEFAULT_SPELLCHECKER_LANG).toString();
}

void SettingsManager::setSpellCheckerLanguage(const QString &lang) {
    if (!lang.trimmed().isEmpty()) {
        settings.setValue(SETTINGS_SPELLCHECKER_LANGUAGE_KEY, lang.trimmed());
    }
}

bool SettingsManager::visualSpellCheckingEnabled() const {
    return settings.value(SETTINGS_VISUAL_SPELLCHECKING_KEY, true).toBool();
}

void SettingsManager::setVisualSpellCheckingEnabled(bool enabled) {
    settings.setValue(SETTINGS_VISUAL_SPELLCHECKING_KEY, enabled);
}

bool SettingsManager::quickFeedbackEnabled() const {
    return settings.value(SETTINGS_QUICK_FEEDBACK_KEY, false).toBool();
}

void SettingsManager::setQuickFeedbackEnabled(bool enabled) {
    settings.setValue(SETTINGS_QUICK_FEEDBACK_KEY, enabled);
} 