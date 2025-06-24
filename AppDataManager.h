#ifndef APPDATAMANAGER_H
#define APPDATAMANAGER_H

#include <QObject>
#include <QString>

class AppDataManager : public QObject {
    Q_OBJECT
public:
    explicit AppDataManager(QObject *parent = nullptr);
    void writeTranslationLog(const QString &inputText);
    void writeMistakesReport(const QString &report);
    static QString getAppDataPath();
    QString getTodaysFileContent() const;
};

#endif // APPDATAMANAGER_H 