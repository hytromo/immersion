#include "AppDataManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QDate>
#include <QDesktopServices>
#include <QUrl>
#include <QtConcurrent>

AppDataManager::AppDataManager(QObject *parent)
    : QObject(parent)
{
}

QString AppDataManager::getAppDataPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

void AppDataManager::writeTranslationLog(const QString &inputText) {
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

QString AppDataManager::getTodaysFileContent() const {
    QString appDataPath = getAppDataPath();
    QString todayFile = appDataPath + "/" + QDate::currentDate().toString("yyyy-MM-dd") + ".txt";
    QFile file(todayFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QString content = file.readAll();
        file.close();
        return content;
    }
    return QString();
}

void AppDataManager::writeMistakesReport(const QString &report) {
    QString appDataPath = getAppDataPath();
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(".");
    }
    
    QString fileName = QDate::currentDate().toString("yyyy-MM-dd") + "-report.txt";
    QString filePath = appDataPath + "/" + fileName;
    
    QFile file(filePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        file.write(report.toUtf8());
        file.close();
        
        // Open the folder automatically
        auto folderUrl = QUrl::fromLocalFile(appDataPath);
        if (folderUrl.isValid()) {
            (void)QtConcurrent::run([folderUrl]() {
                QDesktopServices::openUrl(folderUrl);
            });
        }
    }
} 
