#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "keychainclass.h"
#include "OpenAICommunicator.h"
#include "AppDataManager.h"
#include "SettingsManager.h"
#include "progressdialog.h"

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
#include <QProgressDialog>
#include <QFile>
#include <QDate>
#include <QDialog>
#include <QVBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , keychain(new KeyChainClass(this))
    , appDataManager(new AppDataManager(this))
    , settingsManager(new SettingsManager(this))
    , openaiApiKey("")
{
    ui->setupUi(this);
    ui->inputText->setFocus();

    ui->sourceLang->setText(settingsManager->sourceLang());
    ui->targetLang->setText(settingsManager->targetLang());
    ui->inputText->setPlainText(settingsManager->lastInputText());
    ui->inputText->selectAll();

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this->ui->inputText);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::on_goButton_clicked);

    retrieveOpenAIApiKey();

    connect(ui->actionReset_OpenAI_API_key, SIGNAL(triggered()), this, SLOT(actionReset_OpenAI_API_key()));
    connect(ui->actionOpen_corrections_folder, SIGNAL(triggered()), this, SLOT(actionOpenCorrectionsFolder()));
    connect(ui->actionGenerateMistakesReport, SIGNAL(triggered()), this, SLOT(actionGenerateMistakesReport()));
    connect(ui->actionHelp, SIGNAL(triggered()), this, SLOT(actionHelp()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(actionQuit()));
    connect(ui->actionEditTranslationModel, SIGNAL(triggered()), this, SLOT(actionEditTranslationModel()));
    connect(ui->actionEditReportsModel, SIGNAL(triggered()), this, SLOT(actionEditReportsModel()));
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

void MainWindow::retrieveOpenAIApiKey()
{
    static const QString OPENAI_API_KEY_KEYCHAIN_KEY = "hytromo/immersion/openai_api_key";
    connect(keychain, &KeyChainClass::keyRestored, this,
            [=](const QString &key, const QString &value) {
                openaiApiKey = value;
            });
    connect(keychain, &KeyChainClass::error, this,
            [=](const QString &errorMessage) {
                requestApiKeyPopup();
            });
    keychain->readKey(OPENAI_API_KEY_KEYCHAIN_KEY);
}

void MainWindow::requestApiKeyPopup()
{
    static const QString OPENAI_API_KEY_KEYCHAIN_KEY = "hytromo/immersion/openai_api_key";
    
    ApiKeyDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        openaiApiKey = dialog.getApiKey();
        if (!openaiApiKey.isEmpty()) {
            keychain->writeKey(OPENAI_API_KEY_KEYCHAIN_KEY, openaiApiKey);
        }
    }
}

void MainWindow::actionOpenCorrectionsFolder()
{
    auto appDataPath = AppDataManager::getAppDataPath();
    auto folderUrl = QUrl::fromLocalFile(appDataPath);
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
    openaiApiKey = "";
    requestApiKeyPopup();
}

void MainWindow::cleanupProgressAndCommunicator(QDialog *progress, OpenAICommunicator *communicator) {
    if (progress) {
        progress->close();
        progress->deleteLater();
    }
    if (communicator)
        communicator->deleteLater();

    this->setEnabled(true);
}

void MainWindow::actionGenerateMistakesReport()
{
    if (openaiApiKey.isEmpty()) {
        QMessageBox::warning(this, "Error", "OpenAI API key is missing.");
        return;
    }
    this->setEnabled(false);
    auto progress = new ProgressDialog(this);
    auto openaiCommunicator = new OpenAICommunicator(openaiApiKey, this);
    progress->show();

    auto fileContent = appDataManager->getTodaysFileContent();
    if (fileContent.isEmpty()) {
        cleanupProgressAndCommunicator(progress, openaiCommunicator);
        QMessageBox::warning(this, "Error", "Could not open today's file.");
        return;
    }
    auto sourceLang = ui->sourceLang->text();
    auto prompt = QString("The below file is created by a user still learning %1. Find the top 5 grammatical mistakes and correct them. Provide the original text as-is along with the corrected text and provide a short explanation in English on what kind of mistake the user made. Separate the mistakes by two empty lines in-between. If there aren't enough grammatical errors, feel free to include within the list important spelling mistakes").arg(sourceLang);
    openaiCommunicator->setModelName(settingsManager->reportModelName());
    openaiCommunicator->setPromptRaw(prompt + "\n\n" + fileContent);
    openaiCommunicator->sendRequest();
    connect(openaiCommunicator, &OpenAICommunicator::replyReceived, this, [=](const QString &report) mutable {
        cleanupProgressAndCommunicator(progress, openaiCommunicator);
        appDataManager->writeMistakesReport(report);
    });
    connect(openaiCommunicator, &OpenAICommunicator::errorOccurred, this, [=](const QString &errorString) mutable {
        cleanupProgressAndCommunicator(progress, openaiCommunicator);
        QMessageBox::warning(this, "Network Error", errorString);
    });
}

void MainWindow::on_goButton_clicked()
{
    if (!ui->goButton->isEnabled()) {
        return;
    }
    if (openaiApiKey.isEmpty()) {
        QMessageBox::warning(this, "Error", "OpenAI API key is missing.");
        return;
    }
    ui->goButton->setDisabled(true);
    auto inputText = ui->inputText->toPlainText();
    auto sourceLang = ui->sourceLang->text();
    auto targetLang = ui->targetLang->text();
    auto openaiCommunicator = new OpenAICommunicator(openaiApiKey, this);
    openaiCommunicator->setModelName(settingsManager->translationModelName());
    openaiCommunicator->setPrompt(sourceLang, targetLang, inputText);
    openaiCommunicator->sendRequest();
    connect(openaiCommunicator, &OpenAICommunicator::replyReceived, this, [=](const QString &translation) {
        ui->goButton->setEnabled(true);
        auto clipboard = QGuiApplication::clipboard();
        clipboard->setText(translation);
        appDataManager->writeTranslationLog(ui->inputText->toPlainText());
        this->close();
        openaiCommunicator->deleteLater();
    });
    connect(openaiCommunicator, &OpenAICommunicator::errorOccurred, this, [=](const QString &errorString) {
        ui->goButton->setEnabled(true);
        QMessageBox::warning(this, "Network Error", errorString);
        openaiCommunicator->deleteLater();
    });
}

void MainWindow::actionHelp()
{
    QUrl url("https://github.com/hytromo/immersion");
    QDesktopServices::openUrl(url);
}

void MainWindow::actionQuit()
{
    close();
}

void MainWindow::actionEditTranslationModel()
{
    QString currentModel = settingsManager->translationModelName();
    QString newModel = QInputDialog::getText(this, "Edit Translation Model", 
                                           "Enter the translation model name:", 
                                           QLineEdit::Normal, currentModel);
    if (!newModel.isEmpty() && newModel != currentModel) {
        settingsManager->setTranslationModelName(newModel);
        settingsManager->sync();
    }
}

void MainWindow::actionEditReportsModel()
{
    QString currentModel = settingsManager->reportModelName();
    QString newModel = QInputDialog::getText(this, "Edit Reports Model", 
                                           "Enter the reports model name:", 
                                           QLineEdit::Normal, currentModel);
    if (!newModel.isEmpty() && newModel != currentModel) {
        settingsManager->setReportModelName(newModel);
        settingsManager->sync();
    }
}
