#ifndef APPDATAWRITER_H
#define APPDATAWRITER_H

#include <QObject>
#include <QString>

class AppDataWriter : public QObject {
    Q_OBJECT
public:
    explicit AppDataWriter(QObject *parent = nullptr);
    void writeTranslationLog(const QString &inputText);
    static QString getAppDataPath();
};

#endif // APPDATAWRITER_H 