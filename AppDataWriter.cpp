#include "AppDataWriter.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QFile>

AppDataWriter::AppDataWriter(QObject *parent)
    : QObject(parent)
{
}

QString AppDataWriter::getAppDataPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

void AppDataWriter::writeTranslationLog(const QString &inputText) {
    QString appDataPath = getAppDataPath();
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    QDateTime now = QDateTime::currentDateTime();
    QFile file(appDataPath + "/" + now.toString("yyyy-MM-dd") + ".txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        file.write("\n\n---\n\n");
        file.write(now.toString("HH:mm:ss").toUtf8() + "\n");
        file.write(inputText.toUtf8());
        file.write("\n");
        file.close();
    }
} 
