#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "keychainclass.h"
#include "OpenAICommunicator.h"
#include "AppDataWriter.h"
#include "SettingsManager.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QShortcut>
#include <QClipboard>
#include <QStandardPaths>
#include <QFileInfo>
#include <QDir>
#include <QDateTime>
#include <QThread>
#include <QDesktopServices>
#include <QtConcurrent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , keychain(new KeyChainClass(this))
    , openaiCommunicator(new OpenAICommunicator(this))
    , appDataWriter(new AppDataWriter(this))
    , settingsManager(new SettingsManager(this))
{
    ui->setupUi(this);
    ui->inputText->setFocus();

    ui->sourceLang->setText(settingsManager->sourceLang());
    ui->targetLang->setText(settingsManager->targetLang());
    ui->inputText->setPlainText(settingsManager->lastInputText());
    ui->inputText->selectAll();

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this->ui->inputText);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::on_goButton_clicked);

    static const QString OPENAI_API_KEY_KEYCHAIN_KEY = "hytromo/immersion/openai_api_key";
    connect(keychain, &KeyChainClass::keyRestored, this,
            [=](const QString &key, const QString &value) {
                openaiCommunicator->setApiKey(value);
            });

    connect(keychain, &KeyChainClass::error, this,
            [=](const QString &errorMessage) {
                QString apiKey = "";
                while (apiKey == "") {
                    apiKey = QInputDialog::getText(this, "OpenAI API key missing", "Your API key is missing probably (" + errorMessage + "), please provide it below");
                }
                keychain->writeKey(OPENAI_API_KEY_KEYCHAIN_KEY, apiKey);
            });

    keychain->readKey(OPENAI_API_KEY_KEYCHAIN_KEY);
    connect(ui->actionReset_OpenAI_API_key, SIGNAL(triggered()), this, SLOT(actionReset_OpenAI_API_key()));
    connect(ui->actionOpen_corrections_folder, SIGNAL(triggered()), this, SLOT(actionOpenCorrectionsFolder()));

    connect(openaiCommunicator, &OpenAICommunicator::replyReceived, this, [=](const QString &translation) {
        ui->goButton->setEnabled(true);
        QClipboard *clipboard = QGuiApplication::clipboard();
        clipboard->setText(translation);
        appDataWriter->writeTranslationLog(ui->inputText->toPlainText());
        this->close();
    });
    connect(openaiCommunicator, &OpenAICommunicator::errorOccurred, this, [=](const QString &errorString) {
        ui->goButton->setEnabled(true);
        QMessageBox::warning(this, "Network Error", errorString);
    });
}

MainWindow::~MainWindow()
{
    settingsManager->setSourceLang(ui->sourceLang->text());
    settingsManager->setTargetLang(ui->targetLang->text());
    settingsManager->setLastInputText(ui->inputText->toPlainText());
    settingsManager->sync();
    qDebug() << "Settings saved.";
    delete ui;
}

void MainWindow::actionOpenCorrectionsFolder()
{
    QString appDataPath = AppDataWriter::getAppDataPath();
    QUrl folderUrl = QUrl::fromLocalFile(appDataPath);
    if (!folderUrl.isValid()) {
        QMessageBox::warning(this, "Error", "Invalid folder path: " + appDataPath);
        return;
    }
    (void)QtConcurrent::run([folderUrl]() {
        QDesktopServices::openUrl(folderUrl);
    });
}

void MainWindow::actionReset_OpenAI_API_key()
{
    static const QString OPENAI_API_KEY_KEYCHAIN_KEY = "hytromo/immersion/openai_api_key";
    keychain->deleteKey(OPENAI_API_KEY_KEYCHAIN_KEY);
}

void MainWindow::on_goButton_clicked()
{
    if (!ui->goButton->isEnabled()) {
        return;
    }
    ui->goButton->setDisabled(true);
    QString inputText = ui->inputText->toPlainText();
    QString sourceLang = ui->sourceLang->text();
    QString targetLang = ui->targetLang->text();
    openaiCommunicator->setModelName(settingsManager->modelName());
    openaiCommunicator->setPrompt(sourceLang, targetLang, inputText);
    openaiCommunicator->sendRequest();
}
