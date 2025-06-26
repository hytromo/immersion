#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "keychainclass.h"
#include "OpenAICommunicator.h"
#include "AppDataManager.h"
#include "SettingsManager.h"
#include "ApiKeyDialog.h"
#include "PromptEditDialog.h"

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QSettings>
#include <QProgressDialog>
#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QDesktopServices>
#include <QUrl>
#include <QDate>
#include <QLocale>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class OpenAICommunicator;
class AppDataManager;
class SettingsManager;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_goButton_clicked();
    void actionReset_OpenAI_API_key();
    void actionOpenCorrectionsFolder();
    void actionGenerateMistakesReport();
    void actionHelp();
    void actionQuit();
    void actionEditTranslationModel();
    void actionEditReportsModel();
    void actionEditTranslationPrompt();
    void actionEditReportPrompt();
    void onHistoryActionTriggered();
    void onGenerateReportActionTriggered();

private:
    Ui::MainWindow *ui;
    KeyChainClass *keychain;
    AppDataManager *appDataManager;
    SettingsManager *settingsManager;
    QString openaiApiKey;

    void retrieveOpenAIApiKey();
    void requestApiKeyPopup();
    void cleanupProgressAndCommunicator(QDialog *progress, OpenAICommunicator *communicator);
    void setupHistoryMenu();
    void addMessageToHistory(const QString &message);
    void setupGenerateReportMenu();
    QString formatDateForDisplay(const QDate &date);
    void generateReportForDate(const QString &dateString);
};
#endif // MAINWINDOW_H
