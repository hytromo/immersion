#include "AppDataManager.h"
#include <QStandardPaths>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QDate>
#include <QDesktopServices>
#include <QUrl>
#include <QtConcurrent>
#include <QDebug>

// Constants
namespace {
    const QString DATE_FORMAT = "yyyy-MM-dd";
    const QString TIME_FORMAT = "HH:mm:ss";
    const QString REPORT_SUFFIX = "-report.txt";
    const QString LOG_SEPARATOR = "\n\n---\n\n";
}

AppDataManager::AppDataManager(QObject *parent)
    : QObject(parent)
{
}

QString AppDataManager::getAppDataPath() {
    return QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
}

void AppDataManager::writeTranslationLog(const QString &inputText) {
    if (inputText.trimmed().isEmpty()) {
        qDebug() << "Skipping empty translation log entry";
        return;
    }
    
    QString appDataPath = getAppDataPath();
    QDir dir(appDataPath);
    if (!dir.exists() && !dir.mkpath(".")) {
        qWarning() << "Failed to create app data directory:" << appDataPath;
        return;
    }
    
    QDateTime now = QDateTime::currentDateTime();
    QString filePath = appDataPath + "/" + now.toString(DATE_FORMAT) + ".txt";
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        qWarning() << "Failed to open translation log file:" << filePath;
        return;
    }
    
    file.write(LOG_SEPARATOR.toUtf8());
    file.write(now.toString(TIME_FORMAT).toUtf8() + "\n");
    file.write(inputText.toUtf8());
    file.write("\n");
    file.close();
}

QString AppDataManager::getTodaysFileContent() const {
    return getFileContentForDate(QDate::currentDate().toString(DATE_FORMAT));
}

QString AppDataManager::getFileContentForDate(const QString &dateString) const {
    if (dateString.isEmpty()) {
        qWarning() << "Empty date string provided for file content retrieval";
        return QString();
    }
    
    QString appDataPath = getAppDataPath();
    QString filePath = appDataPath + "/" + dateString + ".txt";
    
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << "Could not open file for date:" << dateString << "at path:" << filePath;
        return QString();
    }
    
    QString content = file.readAll();
    file.close();
    return content;
}

void AppDataManager::writeMistakesReport(const QString &report) {
    writeMistakesReport(report, QDate::currentDate().toString(DATE_FORMAT));
}

void AppDataManager::writeMistakesReport(const QString &report, const QString &dateString) {
    if (report.trimmed().isEmpty()) {
        qWarning() << "Attempted to write empty report for date:" << dateString;
        return;
    }
    
    QString appDataPath = getAppDataPath();
    QDir dir(appDataPath);
    if (!dir.exists() && !dir.mkpath(".")) {
        qWarning() << "Failed to create app data directory:" << appDataPath;
        return;
    }
    
    QString fileName = dateString + REPORT_SUFFIX;
    QString filePath = appDataPath + "/" + fileName;
    
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qWarning() << "Failed to open report file for writing:" << filePath;
        return;
    }
    
    file.write(report.toUtf8());
    file.close();
    
    // Open the folder automatically
    auto folderUrl = QUrl::fromLocalFile(appDataPath);
    if (folderUrl.isValid()) {
        (void)QtConcurrent::run([folderUrl]() {
            QDesktopServices::openUrl(folderUrl);
        });
    } else {
        qWarning() << "Invalid folder URL:" << appDataPath;
    }
} 
