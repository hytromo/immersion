#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "keychainclass.h"
#include "OpenAICommunicator.h"
#include "AppDataManager.h"
#include "SettingsManager.h"
#include "progressdialog.h"
#include "FeedbackDialog.h"

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
#include <QUrl>
#include <QLocale>
#include <QApplication>

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
    connect(ui->actionHelp, SIGNAL(triggered()), this, SLOT(actionHelp()));
    connect(ui->actionQuit, SIGNAL(triggered()), this, SLOT(actionQuit()));
    connect(ui->actionEditTranslationModel, SIGNAL(triggered()), this, SLOT(actionEditTranslationModel()));
    connect(ui->actionEditReportsModel, SIGNAL(triggered()), this, SLOT(actionEditReportsModel()));
    connect(ui->actionEditTranslationPrompt, SIGNAL(triggered()), this, SLOT(actionEditTranslationPrompt()));
    connect(ui->actionEditReportPrompt, SIGNAL(triggered()), this, SLOT(actionEditReportPrompt()));
    connect(ui->actionEditFeedbackPrompt, SIGNAL(triggered()), this, SLOT(actionEditFeedbackPrompt()));
    connect(ui->actionEditFeedbackModel, SIGNAL(triggered()), this, SLOT(actionEditFeedbackModel()));
    
    // Connect to application shutdown signal for graceful shutdown
    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::saveSettings);
    
    setupHistoryMenu();
    setupGenerateReportMenu();
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::saveSettings()
{
    settingsManager->setSourceLang(ui->sourceLang->text());
    settingsManager->setTargetLang(ui->targetLang->text());
    settingsManager->setLastInputText(ui->inputText->toPlainText());
    settingsManager->sync();
    qDebug() << "Settings saved.";
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
    QString promptTemplate = settingsManager->reportPrompt();
    QString prompt = promptTemplate.replace("%sourceLang", sourceLang);
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
    bool quickFeedback = ui->quickFeedbackCheckBox->isChecked();
    
    // Add message to history
    addMessageToHistory(inputText);
    
    auto openaiCommunicator = new OpenAICommunicator(openaiApiKey, this);
    openaiCommunicator->setModelName(settingsManager->translationModelName());
    openaiCommunicator->setPromptWithTemplate(settingsManager->translationPrompt(), sourceLang, targetLang, inputText);
    openaiCommunicator->sendRequest();
    
    connect(openaiCommunicator, &OpenAICommunicator::replyReceived, this, [=](const QString &translation) {
        auto clipboard = QGuiApplication::clipboard();
        clipboard->setText(translation);
        appDataManager->writeTranslationLog(ui->inputText->toPlainText());
        
        // If quick feedback is enabled, request feedback
        if (quickFeedback) {
            auto feedbackCommunicator = new OpenAICommunicator(openaiApiKey, this);
            feedbackCommunicator->setModelName(settingsManager->feedbackModelName());
            
            QString feedbackPromptTemplate = settingsManager->feedbackPrompt();
            QString feedbackPrompt = feedbackPromptTemplate.replace("%sourceLang", sourceLang);
            feedbackCommunicator->setPromptRaw(feedbackPrompt + "\n\n" + inputText);
            feedbackCommunicator->sendRequest();
            
            connect(feedbackCommunicator, &OpenAICommunicator::replyReceived, this, [=](const QString &feedback) {
                ui->goButton->setEnabled(true);
                FeedbackDialog dialog(feedback, this);
                dialog.exec();
                this->close();
                feedbackCommunicator->deleteLater();
            });
            
            connect(feedbackCommunicator, &OpenAICommunicator::errorOccurred, this, [=](const QString &errorString) {
                ui->goButton->setEnabled(true);
                QMessageBox::warning(this, "Feedback Error", "Failed to get feedback: " + errorString);
                this->close();
                feedbackCommunicator->deleteLater();
            });
        } else {
            ui->goButton->setEnabled(true);
            this->close();
        }
        
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

void MainWindow::actionEditTranslationPrompt()
{
    PromptEditDialog dialog(PromptType::Translation, this);
    dialog.setPrompt(settingsManager->translationPrompt());
    
    if (dialog.exec() == QDialog::Accepted) {
        QString newPrompt = dialog.getPrompt();
        if (!newPrompt.isEmpty()) {
            settingsManager->setTranslationPrompt(newPrompt);
            settingsManager->sync();
        }
    }
}

void MainWindow::actionEditReportPrompt()
{
    PromptEditDialog dialog(PromptType::Report, this);
    dialog.setPrompt(settingsManager->reportPrompt());
    
    if (dialog.exec() == QDialog::Accepted) {
        QString newPrompt = dialog.getPrompt();
        if (!newPrompt.isEmpty()) {
            settingsManager->setReportPrompt(newPrompt);
            settingsManager->sync();
        }
    }
}

void MainWindow::actionEditFeedbackPrompt()
{
    PromptEditDialog dialog(PromptType::Feedback, this);
    dialog.setPrompt(settingsManager->feedbackPrompt());
    
    if (dialog.exec() == QDialog::Accepted) {
        QString newPrompt = dialog.getPrompt();
        if (!newPrompt.isEmpty()) {
            settingsManager->setFeedbackPrompt(newPrompt);
            settingsManager->sync();
        }
    }
}

void MainWindow::actionEditFeedbackModel()
{
    QString currentModel = settingsManager->feedbackModelName();
    QString newModel = QInputDialog::getText(this, "Edit Feedback Model", 
                                           "Enter the feedback model name:", 
                                           QLineEdit::Normal, currentModel);
    if (!newModel.isEmpty() && newModel != currentModel) {
        settingsManager->setFeedbackModelName(newModel);
        settingsManager->sync();
    }
}

void MainWindow::setupHistoryMenu()
{
    // Clear existing history actions
    ui->menuHistory->clear();
    
    // Get the message history
    QStringList history = settingsManager->getMessageHistory();
    
    if (history.isEmpty()) {
        // Add a disabled "No history" action
        QAction *noHistoryAction = new QAction("No history", this);
        noHistoryAction->setEnabled(false);
        ui->menuHistory->addAction(noHistoryAction);
    } else {
        // Add history actions
        for (int i = 0; i < history.size(); ++i) {
            QString message = history[i];
            // Truncate long messages for display
            QString displayText = message.length() > 50 ? message.left(47) + "..." : message;
            
            QAction *historyAction = new QAction(displayText, this);
            historyAction->setData(message); // Store the full message
            historyAction->setToolTip(message); // Show full message in tooltip
            
            connect(historyAction, &QAction::triggered, this, &MainWindow::onHistoryActionTriggered);
            ui->menuHistory->addAction(historyAction);
        }
    }
}

void MainWindow::addMessageToHistory(const QString &message)
{
    settingsManager->addMessageToHistory(message);
    setupHistoryMenu(); // Refresh the menu
}

void MainWindow::onHistoryActionTriggered()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action) {
        QString message = action->data().toString();
        if (!message.isEmpty()) {
            ui->inputText->setPlainText(message);
            ui->inputText->selectAll(); // Select all text for easy replacement
        }
    }
}

void MainWindow::setupGenerateReportMenu()
{
    // Clear existing report actions
    ui->menuGenerateReport->clear();
    
    // Get the app data path
    QString appDataPath = AppDataManager::getAppDataPath();
    QDir appDataDir(appDataPath);
    
    // Get all .txt files in the app data directory
    QStringList filters;
    filters << "*.txt";
    QFileInfoList files = appDataDir.entryInfoList(filters, QDir::Files, QDir::Time);
    
    // Create a list of available dates (last 10 days)
    QList<QDate> availableDates;
    QDate today = QDate::currentDate();
    
    // Check the last 10 days
    for (int i = 0; i < 10; ++i) {
        QDate checkDate = today.addDays(-i);
        QString expectedFileName = checkDate.toString("yyyy-MM-dd") + ".txt";
        
        // Check if file exists
        bool fileExists = false;
        for (const QFileInfo &fileInfo : files) {
            if (fileInfo.fileName() == expectedFileName) {
                fileExists = true;
                break;
            }
        }
        
        if (fileExists) {
            availableDates.append(checkDate);
        }
    }
    
    if (availableDates.isEmpty()) {
        // Add a disabled "No data available" action
        QAction *noDataAction = new QAction("No data available", this);
        noDataAction->setEnabled(false);
        ui->menuGenerateReport->addAction(noDataAction);
    } else {
        // Add actions for each available date
        for (const QDate &date : availableDates) {
            QString displayText = formatDateForDisplay(date);
            QAction *reportAction = new QAction(displayText, this);
            reportAction->setData(date.toString("yyyy-MM-dd")); // Store the date as data
            
            connect(reportAction, &QAction::triggered, this, &MainWindow::onGenerateReportActionTriggered);
            ui->menuGenerateReport->addAction(reportAction);
        }
    }
}

QString MainWindow::formatDateForDisplay(const QDate &date)
{
    int day = date.day();
    QString daySuffix;
    
    // Determine the correct suffix for the day
    if (day >= 11 && day <= 13) {
        daySuffix = "th";
    } else {
        switch (day % 10) {
            case 1: daySuffix = "st"; break;
            case 2: daySuffix = "nd"; break;
            case 3: daySuffix = "rd"; break;
            default: daySuffix = "th"; break;
        }
    }
    
    QLocale locale;
    QString monthName = locale.monthName(date.month(), QLocale::LongFormat);
    int year = date.year();
    
    return QString("%1%2 of %3 %4").arg(day).arg(daySuffix).arg(monthName).arg(year);
}

void MainWindow::onGenerateReportActionTriggered()
{
    QAction *action = qobject_cast<QAction*>(sender());
    if (action) {
        QString dateString = action->data().toString();
        if (!dateString.isEmpty()) {
            generateReportForDate(dateString);
        }
    }
}

void MainWindow::generateReportForDate(const QString &dateString)
{
    if (openaiApiKey.isEmpty()) {
        QMessageBox::warning(this, "Error", "OpenAI API key is missing.");
        return;
    }
    
    this->setEnabled(false);
    auto progress = new ProgressDialog(this);
    auto openaiCommunicator = new OpenAICommunicator(openaiApiKey, this);
    progress->show();

    auto fileContent = appDataManager->getFileContentForDate(dateString);
    if (fileContent.isEmpty()) {
        cleanupProgressAndCommunicator(progress, openaiCommunicator);
        QMessageBox::warning(this, "Error", "Could not open file for " + dateString + ".");
        return;
    }
    
    auto sourceLang = ui->sourceLang->text();
    QString promptTemplate = settingsManager->reportPrompt();
    QString prompt = promptTemplate.replace("%sourceLang", sourceLang);
    openaiCommunicator->setModelName(settingsManager->reportModelName());
    openaiCommunicator->setPromptRaw(prompt + "\n\n" + fileContent);
    openaiCommunicator->sendRequest();
    
    connect(openaiCommunicator, &OpenAICommunicator::replyReceived, this, [=](const QString &report) mutable {
        cleanupProgressAndCommunicator(progress, openaiCommunicator);
        appDataManager->writeMistakesReport(report, dateString);
    });
    
    connect(openaiCommunicator, &OpenAICommunicator::errorOccurred, this, [=](const QString &errorString) mutable {
        cleanupProgressAndCommunicator(progress, openaiCommunicator);
        QMessageBox::warning(this, "Network Error", errorString);
    });
}
