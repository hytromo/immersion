#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "keychainclass.h"

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QSettings>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    static const QString SETTINGS_MODEL_NAME_KEY;
    static const QString SETTINGS_SOURCE_LANG_KEY;
    static const QString SETTINGS_TARGET_LANG_KEY;
    static const QString SETTINGS_LAST_INPUT_KEY;
    static const QString OPENAI_API_KEY_KEYCHAIN_KEY;
    QString openaiApiKey;
    QString lastInputText;

private slots:
    void on_goButton_clicked();
    void actionReset_OpenAI_API_key();
    void handleNetworkReply(QNetworkReply *reply);
private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    QSettings *settings;
    KeyChainClass *keychain;

};
#endif // MAINWINDOW_H
