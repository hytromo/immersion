#ifndef SETTINGSMANAGER_H
#define SETTINGSMANAGER_H

#include <QObject>
#include <QString>
#include <QSettings>

class SettingsManager : public QObject {
    Q_OBJECT
public:
    explicit SettingsManager(QObject *parent = nullptr);
    QString modelName() const;
    void setModelName(const QString &name);
    QString sourceLang() const;
    void setSourceLang(const QString &lang);
    QString targetLang() const;
    void setTargetLang(const QString &lang);
    QString lastInputText() const;
    void setLastInputText(const QString &text);
    void sync();
private:
    QSettings settings;
};

#endif // SETTINGSMANAGER_H 