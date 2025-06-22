#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "keychainclass.h"

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

const QString MainWindow::SETTINGS_MODEL_NAME_KEY = "model_name";
const QString MainWindow::SETTINGS_SOURCE_LANG_KEY = "sourceLang";
const QString MainWindow::SETTINGS_TARGET_LANG_KEY = "targetLang";
const QString MainWindow::SETTINGS_LAST_INPUT_KEY = "lastInputText";
const QString MainWindow::OPENAI_API_KEY_KEYCHAIN_KEY = "hytromo/immersion/openai_api_key";


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , networkManager(new QNetworkAccessManager(this))
    , settings(new QSettings(this))
    , keychain(new KeyChainClass(this))
{
    ui->setupUi(this);
    ui->inputText->setFocus();

    connect(networkManager, &QNetworkAccessManager::finished,
            this, &MainWindow::handleNetworkReply);

    ui->sourceLang->setText(settings->value(MainWindow::SETTINGS_SOURCE_LANG_KEY, "Danish").toString());
    ui->targetLang->setText(settings->value(MainWindow::SETTINGS_TARGET_LANG_KEY, "English").toString());
    ui->inputText->setPlainText(settings->value(MainWindow::SETTINGS_LAST_INPUT_KEY, "").toString());
    ui->inputText->selectAll();

    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this->ui->inputText);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::on_goButton_clicked);

    connect(keychain, &KeyChainClass::keyRestored, this,
            [=](const QString &key, const QString &value) {
                this->openaiApiKey = value;
            });

    connect(keychain, &KeyChainClass::error, this,
            [=](const QString &errorMessage) {
                QString apiKey = "";
                while (apiKey == "") {
                    apiKey = QInputDialog::getText(this, "OpenAI API key missing", "Your API key is missing probably (" + errorMessage + "), please provide it below");
                }
                keychain->writeKey(MainWindow::OPENAI_API_KEY_KEYCHAIN_KEY, apiKey);
            });

    keychain->readKey(MainWindow::OPENAI_API_KEY_KEYCHAIN_KEY);
    connect(ui->actionReset_OpenAI_API_key, SIGNAL(triggered()), this, SLOT(actionReset_OpenAI_API_key()));
    connect(ui->actionOpen_corrections_folder, SIGNAL(triggered()), this, SLOT(actionOpenCorrectionsFolder()));

}

MainWindow::~MainWindow()
{
    settings->setValue(MainWindow::SETTINGS_SOURCE_LANG_KEY, ui->sourceLang->text());
    settings->setValue(MainWindow::SETTINGS_TARGET_LANG_KEY, ui->targetLang->text());
    settings->setValue(MainWindow::SETTINGS_LAST_INPUT_KEY, ui->inputText->toPlainText());
    settings->setValue(MainWindow::SETTINGS_MODEL_NAME_KEY, settings->value(MainWindow::SETTINGS_MODEL_NAME_KEY, "gpt-4o-mini"));

    qDebug() << "Settings saved at" << settings->fileName();

    delete ui;
}

void MainWindow::actionOpenCorrectionsFolder()
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QUrl folderUrl = QUrl::fromLocalFile(appDataPath);

    if (!folderUrl.isValid()) {
        QMessageBox::warning(this, "Error", "Invalid folder path: " + appDataPath);
        return;
    }

    (void)QtConcurrent::run([folderUrl]() {
        QDesktopServices::openUrl(folderUrl);
    });}

void MainWindow::actionReset_OpenAI_API_key()
{
    keychain->deleteKey(MainWindow::OPENAI_API_KEY_KEYCHAIN_KEY);
}

void MainWindow::on_goButton_clicked()
{
    QString inputText = ui->inputText->toPlainText();
    QString sourceLang = ui->sourceLang->text();
    QString targetLang = ui->targetLang->text();
    this->lastInputText = inputText;

    QJsonObject json;
    json["model"] = settings->value(MainWindow::SETTINGS_MODEL_NAME_KEY, "gpt-4o-mini").toString();

    QJsonArray messages;
    QJsonObject message;
    message["role"] = "user";
    message["content"] = QString(
                             "Translate the following from %1 to %2. "
                             "Also return the original sentence with grammar/spelling corrections marked with * around them.")
                             .arg(sourceLang, targetLang);
    messages.append(message);
    messages.append(QJsonObject{
        {"role", "user"},
        {"content", inputText}
    });
    json["messages"] = messages;

    QJsonObject schema;
    schema["type"] = "object";
    schema["properties"] = QJsonObject{
        {"translation", QJsonObject{{"type", "string"}}},
        {"originalWithCorrections", QJsonObject{{"type", "string"}}}
    };
    schema["required"] = QJsonArray{"translation", "originalWithCorrections"};
    schema["additionalProperties"] = false;

    json["response_format"] = QJsonObject{
        {"type", "json_schema"},
        {"json_schema", QJsonObject{
                            {"name", "translation_response"},
                            {"strict", true},
                            {"schema", schema}
                        }}
    };

    QNetworkRequest request(QUrl("https://api.openai.com/v1/chat/completions"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Authorization", ("Bearer " + this->openaiApiKey).toUtf8());

    networkManager->post(request, QJsonDocument(json).toJson());
}

void MainWindow::handleNetworkReply(QNetworkReply *reply)
{
    QByteArray responseData = reply->readAll();
    qDebug() << "\n-- Response Body --\n" << responseData;

    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::warning(this, "Network Error", reply->errorString());
        reply->deleteLater();
        return;
    }

    QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
    QJsonObject root = jsonDoc.object();

    QJsonArray choices = root["choices"].toArray();
    if (choices.isEmpty()) {
        QMessageBox::warning(this, "Error", "No choices returned.");
        reply->deleteLater();
        return;
    }

    QJsonObject messageObj = choices[0].toObject()["message"].toObject();
    QString contentStr = messageObj["content"].toString();

    // Parse structured content
    QJsonDocument contentDoc = QJsonDocument::fromJson(contentStr.toUtf8());
    if (!contentDoc.isObject()) {
        QMessageBox::warning(this, "Error", "Failed to parse structured JSON.");
        reply->deleteLater();
        return;
    }

    QJsonObject result = contentDoc.object();
    QString translation = result["translation"].toString();
    QString corrected = result["originalWithCorrections"].toString();

    reply->deleteLater();

    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(translation);

    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);

    QDir dir(appDataPath);
    if (!dir.exists()){
        dir.mkpath(".");
    }

    QDateTime now = QDateTime::currentDateTime();
    QFile file(appDataPath + "/" + now.toString("yyyy-MM-dd") + ".txt");
    if (file.open(QIODevice::WriteOnly | QIODevice::Append)) {
        file.write("\n\n---\n\n");
        file.write(now.toString("HH:mm").toUtf8());
        file.write("\noriginal: ");
        file.write(this->lastInputText.toUtf8());
        file.write("\n\ncorrected: ");
        file.write(corrected.toUtf8());
        file.close();
    }

    this->close();
}

