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
    void writeMistakesReport(const QString &report, const QString &dateString);
    static QString getAppDataPath();
    QString getTodaysFileContent() const;
    QString getFileContentForDate(const QString &dateString) const;
};

#endif // APPDATAMANAGER_H 