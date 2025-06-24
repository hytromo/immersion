#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "keychainclass.h"
#include "OpenAICommunicator.h"
#include "AppDataManager.h"
#include "SettingsManager.h"

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QSettings>
#include <QProgressDialog>

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

private:
    Ui::MainWindow *ui;
    KeyChainClass *keychain;
    AppDataManager *appDataManager;
    SettingsManager *settingsManager;
    QString openaiApiKey;

    void retrieveOpenAIApiKey();
    void cleanupProgressAndCommunicator(QProgressDialog *progress, OpenAICommunicator *communicator, bool reenableWindow = true);
};
#endif // MAINWINDOW_H
