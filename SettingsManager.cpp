#include "SettingsManager.h"

const QString SETTINGS_MODEL_NAME_KEY = "model_name";
const QString SETTINGS_SOURCE_LANG_KEY = "sourceLang";
const QString SETTINGS_TARGET_LANG_KEY = "targetLang";
const QString SETTINGS_LAST_INPUT_KEY = "lastInputText";

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent), settings(parent)
{
}

QString SettingsManager::modelName() const {
    return settings.value(SETTINGS_MODEL_NAME_KEY, "gpt-4o-mini").toString();
}
void SettingsManager::setModelName(const QString &name) {
    settings.setValue(SETTINGS_MODEL_NAME_KEY, name);
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
void SettingsManager::sync() {
    settings.sync();
} 