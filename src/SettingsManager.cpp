#include "SettingsManager.h"

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
const int MAX_HISTORY_SIZE = 5;
const QString SETTINGS_SPELLCHECKER_LANGUAGE_KEY = "spellCheckerLanguage";
const QString SETTINGS_VISUAL_SPELLCHECKING_KEY = "visualSpellCheckingEnabled";
const QString SETTINGS_QUICK_FEEDBACK_KEY = "quickFeedbackEnabled";

// Default prompts
const QString DEFAULT_TRANSLATION_PROMPT = "You are an expert %sourceLang to %targetLang translator. Translate this text making sure to match the tone and style of the original.";
const QString DEFAULT_REPORT_PROMPT = "You are an expert %sourceLang teacher. Find the top 5 grammatical mistakes in this %sourceLang text and correct them. Format each mistake as:\n\nORIGINAL: [mistake]\nCORRECTED: [correction]\nEXPLANATION: [brief English explanation]\n\nSeparate entries with two empty lines. If fewer than 5 grammatical errors exist, include important spelling mistakes.";
const QString DEFAULT_FEEDBACK_PROMPT = "You are an expert %sourceLang teacher. Provide feedback on the syntax, grammar, and fluency of this %sourceLang text. Be constructive and specific. Format your response as:\n\nSYNTAX: [feedback on sentence structure]\nGRAMMAR: [feedback on grammatical correctness]\nFLUENCY: [feedback on naturalness and flow]\n\nKeep each section concise but helpful.";

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent), settings(parent)
{
}

QString SettingsManager::translationModelName() const {
    return settings.value(SETTINGS_TRANSLATION_MODEL_NAME_KEY, "gpt-4o-mini").toString();
}
void SettingsManager::setTranslationModelName(const QString &name) {
    settings.setValue(SETTINGS_TRANSLATION_MODEL_NAME_KEY, name);
}
QString SettingsManager::reportModelName() const {
    return settings.value(SETTINGS_REPORT_MODEL_NAME_KEY, "gpt-4.1").toString();
}
void SettingsManager::setReportModelName(const QString &name) {
    settings.setValue(SETTINGS_REPORT_MODEL_NAME_KEY, name);
}
QString SettingsManager::feedbackModelName() const {
    return settings.value(SETTINGS_FEEDBACK_MODEL_NAME_KEY, "o3").toString();
}
void SettingsManager::setFeedbackModelName(const QString &name) {
    settings.setValue(SETTINGS_FEEDBACK_MODEL_NAME_KEY, name);
}
QString SettingsManager::sourceLang() const {
    return settings.value(SETTINGS_SOURCE_LANG_KEY, "Danish").toString();
}
void SettingsManager::setSourceLang(const QString &lang) {
    settings.setValue(SETTINGS_SOURCE_LANG_KEY, lang);
}
QString SettingsManager::targetLang() const {
    return settings.value(SETTINGS_TARGET_LANG_KEY, "English").toString();
}
void SettingsManager::setTargetLang(const QString &lang) {
    settings.setValue(SETTINGS_TARGET_LANG_KEY, lang);
}
QString SettingsManager::lastInputText() const {
    return settings.value(SETTINGS_LAST_INPUT_KEY, "").toString();
}
void SettingsManager::setLastInputText(const QString &text) {
    settings.setValue(SETTINGS_LAST_INPUT_KEY, text);
}

QString SettingsManager::translationPrompt() const {
    return settings.value(SETTINGS_TRANSLATION_PROMPT_KEY, getDefaultTranslationPrompt()).toString();
}
void SettingsManager::setTranslationPrompt(const QString &prompt) {
    settings.setValue(SETTINGS_TRANSLATION_PROMPT_KEY, prompt);
}

QString SettingsManager::reportPrompt() const {
    return settings.value(SETTINGS_REPORT_PROMPT_KEY, getDefaultReportPrompt()).toString();
}
void SettingsManager::setReportPrompt(const QString &prompt) {
    settings.setValue(SETTINGS_REPORT_PROMPT_KEY, prompt);
}

QString SettingsManager::feedbackPrompt() const {
    return settings.value(SETTINGS_FEEDBACK_PROMPT_KEY, getDefaultFeedbackPrompt()).toString();
}
void SettingsManager::setFeedbackPrompt(const QString &prompt) {
    settings.setValue(SETTINGS_FEEDBACK_PROMPT_KEY, prompt);
}

QString SettingsManager::getDefaultTranslationPrompt() const {
    return DEFAULT_TRANSLATION_PROMPT;
}

QString SettingsManager::getDefaultReportPrompt() const {
    return DEFAULT_REPORT_PROMPT;
}

QString SettingsManager::getDefaultFeedbackPrompt() const {
    return DEFAULT_FEEDBACK_PROMPT;
}

QStringList SettingsManager::getMessageHistory() const {
    QVariant historyVariant = settings.value(SETTINGS_MESSAGE_HISTORY_KEY);
    if (historyVariant.canConvert<QStringList>()) {
        return historyVariant.toStringList();
    }
    return QStringList();
}

void SettingsManager::addMessageToHistory(const QString &message) {
    if (message.trimmed().isEmpty()) {
        return; // Don't add empty messages
    }
    
    QStringList history = getMessageHistory();
    
    // Remove the message if it already exists (to avoid duplicates)
    history.removeAll(message);
    
    // Add the new message at the beginning
    history.prepend(message);
    
    // Keep only the last MAX_HISTORY_SIZE messages
    if (history.size() > MAX_HISTORY_SIZE) {
        history = history.mid(0, MAX_HISTORY_SIZE);
    }
    
    settings.setValue(SETTINGS_MESSAGE_HISTORY_KEY, QVariant(history));
}

void SettingsManager::sync() {
    settings.sync();
}

QString SettingsManager::spellCheckerLanguage() const {
    return settings.value(SETTINGS_SPELLCHECKER_LANGUAGE_KEY, "en_US").toString();
}
void SettingsManager::setSpellCheckerLanguage(const QString &lang) {
    settings.setValue(SETTINGS_SPELLCHECKER_LANGUAGE_KEY, lang);
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