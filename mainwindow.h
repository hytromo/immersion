#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "keychainclass.h"
#include "OpenAICommunicator.h"
#include "AppDataWriter.h"
#include "SettingsManager.h"

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class OpenAICommunicator;
class AppDataWriter;
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
private:
    Ui::MainWindow *ui;
    KeyChainClass *keychain;
    OpenAICommunicator *openaiCommunicator;
    AppDataWriter *appDataWriter;
    SettingsManager *settingsManager;

};
#endif // MAINWINDOW_H
