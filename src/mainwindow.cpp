#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "KeychainManager.h"
#include "OpenAICommunicator.h"
#include "AppDataManager.h"
#include "SettingsManager.h"
#include "ProgressDialog.h"
#include "FeedbackDialog.h"
#include "SpellChecker.h"
#include "NetworkRequestManager.h"
#include "StringUtils.h"

#include <QInputDialog>
#include <QMessageBox>
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

// Constants
const QString MainWindow::OPENAI_API_KEY_KEYCHAIN_KEY = "hytromo/immersion/openai_api_key";
const int MainWindow::MAX_HISTORY_SIZE = 5;
const int MainWindow::SPELL_CHECK_DELAY_MS = 500;
const int MainWindow::MAX_HISTORY_DISPLAY_LENGTH = 50;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , keychain(new KeychainManager(this))
    , appDataManager(new AppDataManager(this))
    , settingsManager(new SettingsManager(this))
    , spellChecker(new SpellChecker(this))
    , networkManager(new NetworkRequestManager(this))
    , openaiApiKey("")
{
    ui->setupUi(this);
    ui->inputText->setFocus();

    initializeUI();
    setupConnections();
    setupHistoryMenu();
    setupGenerateReportMenu();
    setupSpellChecker();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initializeUI()
{
    ui->sourceLang->setText(settingsManager->sourceLang());
    ui->targetLang->setText(settingsManager->targetLang());
    ui->inputText->setPlainText(settingsManager->lastInputText());
    ui->inputText->selectAll();

    ui->quickFeedbackCheckBox->setChecked(settingsManager->quickFeedbackEnabled());
}

void MainWindow::setupConnections()
{
    // Keyboard shortcuts
    QShortcut *shortcut = new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this->ui->inputText);
    connect(shortcut, &QShortcut::activated, this, &MainWindow::on_goButton_clicked);

    // Menu actions
    connect(ui->actionReset_OpenAI_API_key, &QAction::triggered, this, &MainWindow::actionReset_OpenAI_API_key);
    connect(ui->actionOpen_corrections_folder, &QAction::triggered, this, &MainWindow::actionOpenCorrectionsFolder);
    connect(ui->actionHelp, &QAction::triggered, this, &MainWindow::actionHelp);
    connect(ui->actionQuit, &QAction::triggered, this, &MainWindow::actionQuit);
    connect(ui->actionEditTranslationModel, &QAction::triggered, this, &MainWindow::actionEditTranslationModel);
    connect(ui->actionEditReportsModel, &QAction::triggered, this, &MainWindow::actionEditReportsModel);
    connect(ui->actionEditTranslationPrompt, &QAction::triggered, this, &MainWindow::actionEditTranslationPrompt);
    connect(ui->actionEditReportPrompt, &QAction::triggered, this, &MainWindow::actionEditReportPrompt);
    connect(ui->actionEditFeedbackPrompt, &QAction::triggered, this, &MainWindow::actionEditFeedbackPrompt);
    connect(ui->actionEditFeedbackModel, &QAction::triggered, this, &MainWindow::actionEditFeedbackModel);
    connect(ui->actionEditSpellCheckerLanguage, &QAction::triggered, this, &MainWindow::actionEditSpellCheckerLanguage);
    connect(ui->actionToggleVisualSpellChecking, &QAction::triggered, this, &MainWindow::actionToggleVisualSpellChecking);
    
    // Application shutdown
    connect(qApp, &QApplication::aboutToQuit, this, &MainWindow::saveSettings);
    
    // Quick feedback checkbox
    connect(ui->quickFeedbackCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        settingsManager->setQuickFeedbackEnabled(checked);
        settingsManager->sync();
    });

    retrieveOpenAIApiKey();
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
    connect(keychain, &KeychainManager::keyRestored, this,
            [this](const QString &key, const QString &value) {
                openaiApiKey = value;
            });
    connect(keychain, &KeychainManager::error, this,
            [this](const QString &errorMessage) {
                requestApiKeyPopup();
            });
    keychain->readKey(OPENAI_API_KEY_KEYCHAIN_KEY);
}

void MainWindow::requestApiKeyPopup()
{
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
    const QString appDataPath = AppDataManager::getAppDataPath();
    const QUrl folderUrl = QUrl::fromLocalFile(appDataPath);
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
    keychain->deleteKey(OPENAI_API_KEY_KEYCHAIN_KEY);
    openaiApiKey.clear();
    requestApiKeyPopup();
}

void MainWindow::actionGenerateMistakesReport()
{
    if (!validateApiKey()) {
        return;
    }
    
    const QString fileContent = appDataManager->getTodaysFileContent();
    if (fileContent.isEmpty()) {
        QMessageBox::warning(this, "Error", "Could not open today's file.");
        return;
    }
    
    const QString sourceLang = ui->sourceLang->text();
    QString promptTemplate = settingsManager->reportPrompt();
    const QString prompt = promptTemplate.replace("%sourceLang", sourceLang);
    
    networkManager->executeReportRequest(openaiApiKey, 
                                       settingsManager->reportModelName(),
                                       prompt,
                                       fileContent,
                                       this,
                                       [this](const QString &report) {
                                           appDataManager->writeMistakesReport(report);
                                       },
                                       [this](const QString &error) {
                                           QMessageBox::warning(this, "Error", error);
                                       });
}

bool MainWindow::validateApiKey()
{
    if (openaiApiKey.isEmpty()) {
        QMessageBox::warning(this, "Error", "OpenAI API key is missing.");
        return false;
    }
    return true;
}

void MainWindow::on_goButton_clicked()
{
    if (!ui->goButton->isEnabled()) {
        return;
    }
    
    if (!validateApiKey()) {
        return;
    }
    
    ui->goButton->setDisabled(true);
    const QString inputText = ui->inputText->toPlainText();
    const QString sourceLang = ui->sourceLang->text();
    const QString targetLang = ui->targetLang->text();
    const bool quickFeedback = ui->quickFeedbackCheckBox->isChecked();
    
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
            requestQuickFeedback(inputText, sourceLang);
        } else {
            ui->goButton->setEnabled(true);
            this->hide();
        }
        
        openaiCommunicator->deleteLater();
    });
    
    connect(openaiCommunicator, &OpenAICommunicator::errorOccurred, this, [=](const QString &errorString) {
        ui->goButton->setEnabled(true);
        QMessageBox::warning(this, "Network Error", errorString);
        openaiCommunicator->deleteLater();
    });
}

void MainWindow::requestQuickFeedback(const QString &inputText, const QString &sourceLang)
{
    auto feedbackCommunicator = new OpenAICommunicator(openaiApiKey, this);
    feedbackCommunicator->setModelName(settingsManager->feedbackModelName());
    
    QString feedbackPromptTemplate = settingsManager->feedbackPrompt();
    const QString feedbackPrompt = feedbackPromptTemplate.replace("%sourceLang", sourceLang);
    feedbackCommunicator->setPromptRaw(feedbackPrompt + "\n\n" + inputText);
    feedbackCommunicator->sendRequest();
    
    connect(feedbackCommunicator, &OpenAICommunicator::replyReceived, this, [=](const QString &feedback) {
        ui->goButton->setEnabled(true);
        FeedbackDialog dialog(feedback, this);
        dialog.exec();
        this->hide();
        feedbackCommunicator->deleteLater();
    });
    
    connect(feedbackCommunicator, &OpenAICommunicator::errorOccurred, this, [=](const QString &errorString) {
        ui->goButton->setEnabled(true);
        QMessageBox::warning(this, "Feedback Error", "Failed to get feedback: " + errorString);
        this->hide();
        feedbackCommunicator->deleteLater();
    });
}

void MainWindow::actionHelp()
{
    const QUrl url("https://github.com/hytromo/immersion");
    QDesktopServices::openUrl(url);
}

void MainWindow::actionQuit()
{
    close();
}

void MainWindow::actionEditTranslationModel()
{
    editModelSetting(settingsManager->translationModelName(), 
                    "Edit Translation Model", 
                    &SettingsManager::setTranslationModelName);
}

void MainWindow::actionEditReportsModel()
{
    editModelSetting(settingsManager->reportModelName(), 
                    "Edit Reports Model", 
                    &SettingsManager::setReportModelName);
}

void MainWindow::actionEditTranslationPrompt()
{
    editPromptSetting(PromptType::Translation, 
                     settingsManager->translationPrompt(), 
                     &SettingsManager::setTranslationPrompt);
}

void MainWindow::actionEditReportPrompt()
{
    editPromptSetting(PromptType::Report, 
                     settingsManager->reportPrompt(), 
                     &SettingsManager::setReportPrompt);
}

void MainWindow::actionEditFeedbackPrompt()
{
    editPromptSetting(PromptType::Feedback, 
                     settingsManager->feedbackPrompt(), 
                     &SettingsManager::setFeedbackPrompt);
}

void MainWindow::actionEditFeedbackModel()
{
    editModelSetting(settingsManager->feedbackModelName(), 
                    "Edit quick feedback model", 
                    &SettingsManager::setFeedbackModelName);
}

void MainWindow::actionEditSpellCheckerLanguage()
{
    if (!spellChecker->isSpellCheckingAvailable()) {
        QMessageBox::information(this, "Spell Checker", "Spell checking is not available on this platform.");
        return;
    }
    
    const QStringList availableLanguages = spellChecker->getAvailableLanguages();
    if (availableLanguages.isEmpty()) {
        QMessageBox::information(this, "Spell Checker", "No spell checker languages available.");
        return;
    }
    
    const QString currentLanguage = spellChecker->getCurrentLanguage();
    int currentIndex = availableLanguages.indexOf(currentLanguage);
    if (currentIndex == -1) {
        currentIndex = 0;
    }
    
    bool ok;
    const QString newLanguage = QInputDialog::getItem(this, "Edit Spell Checker Language",
                                              "Choose spell checker language:", availableLanguages,
                                              currentIndex, false, &ok);
    if (ok && !newLanguage.isEmpty() && newLanguage != currentLanguage) {
        spellChecker->setLanguage(newLanguage);
        settingsManager->setSpellCheckerLanguage(newLanguage);
        settingsManager->sync();
        qDebug() << "Spell checker language changed to:" << newLanguage;
        updateSpellCheckerStatusLabel();
    }
}

void MainWindow::actionToggleVisualSpellChecking()
{
    if (!spellChecker->isSpellCheckingAvailable()) {
        QMessageBox::information(this, "Spell Checker", "Spell checking is not available on this platform.");
        return;
    }
    
    const bool currentlyEnabled = spellChecker->isVisualSpellCheckingEnabled();
    spellChecker->enableVisualSpellChecking(!currentlyEnabled);
    settingsManager->setVisualSpellCheckingEnabled(!currentlyEnabled);
    settingsManager->sync();
    updateSpellCheckerStatusLabel();
}

void MainWindow::setupHistoryMenu()
{
    // Clear existing history actions
    ui->menuHistory->clear();
    
    // Get the message history
    const QStringList history = settingsManager->getMessageHistory();
    
    if (history.isEmpty()) {
        // Add a disabled "No history" action
        QAction *noHistoryAction = new QAction("No history", this);
        noHistoryAction->setEnabled(false);
        ui->menuHistory->addAction(noHistoryAction);
    } else {
        // Add history actions
        for (const QString &message : history) {
            // Truncate long messages for display
            const QString displayText = StringUtils::truncateWithEllipsis(message, MAX_HISTORY_DISPLAY_LENGTH);
            
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
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        const QString message = action->data().toString();
        if (!message.isEmpty()) {
            ui->inputText->setPlainText(message);
            ui->inputText->selectAll(); // Select all text for easy replacement
        }
    }
}

void MainWindow::onGenerateReportActionTriggered()
{
    if (QAction *action = qobject_cast<QAction*>(sender())) {
        const QString dateString = action->data().toString();
        if (!dateString.isEmpty()) {
            generateReportForDate(dateString);
        }
    }
}

void MainWindow::setupGenerateReportMenu()
{
    // Clear existing report actions
    ui->menuGenerateReport->clear();
    
    // Get the app data path
    const QString appDataPath = AppDataManager::getAppDataPath();
    const QDir appDataDir(appDataPath);
    
    // Get all .txt files in the app data directory
    const QStringList filters{"*.txt"};
    const QFileInfoList files = appDataDir.entryInfoList(filters, QDir::Files, QDir::Time);
    
    // Create a list of available dates (last 10 days)
    QList<QDate> availableDates;
    const QDate today = QDate::currentDate();
    
    // Check the last 10 days
    for (int i = 0; i < 10; ++i) {
        const QDate checkDate = today.addDays(-i);
        const QString expectedFileName = checkDate.toString("yyyy-MM-dd") + ".txt";
        
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
            const QString displayText = StringUtils::formatDateWithOrdinal(date);
            QAction *reportAction = new QAction(displayText, this);
            reportAction->setData(date.toString("yyyy-MM-dd")); // Store the date as data
            
            connect(reportAction, &QAction::triggered, this, &MainWindow::onGenerateReportActionTriggered);
            ui->menuGenerateReport->addAction(reportAction);
        }
    }
}

void MainWindow::generateReportForDate(const QString &dateString)
{
    if (!validateApiKey()) {
        return;
    }
    
    const QString fileContent = appDataManager->getFileContentForDate(dateString);
    if (fileContent.isEmpty()) {
        QMessageBox::warning(this, "Error", "Could not open file for " + dateString + ".");
        return;
    }
    
    const QString sourceLang = ui->sourceLang->text();
    QString promptTemplate = settingsManager->reportPrompt();
    const QString prompt = promptTemplate.replace("%sourceLang", sourceLang);
    
    networkManager->executeReportRequest(openaiApiKey, 
                                       settingsManager->reportModelName(),
                                       prompt,
                                       fileContent,
                                       this,
                                       [this, dateString](const QString &report) {
                                           appDataManager->writeMistakesReport(report, dateString);
                                       },
                                       [this](const QString &error) {
                                           QMessageBox::warning(this, "Error", error);
                                       });
}

void MainWindow::setupSpellChecker()
{
    if (spellChecker->isSpellCheckingAvailable()) {
        // Enable spell checking for the input text area
        spellChecker->enableSpellChecking(ui->inputText);
        
        // Load saved settings
        const QString savedSpellLang = settingsManager->spellCheckerLanguage();
        const bool savedVisual = settingsManager->visualSpellCheckingEnabled();
        
        // Enable visual spell checking (red underlines)
        spellChecker->enableVisualSpellChecking(savedVisual);
        
        // Set the language based on the source language
        const QString sourceLang = ui->sourceLang->text();
        const QString spellCheckLang = mapLanguageToSpellCheckLanguage(sourceLang);
        spellChecker->setLanguage(savedSpellLang.isEmpty() ? spellCheckLang : savedSpellLang);
        
        // Update status label
        updateSpellCheckerStatusLabel();
        
        settingsManager->setSpellCheckerLanguage(spellChecker->getCurrentLanguage());
        settingsManager->setVisualSpellCheckingEnabled(spellChecker->isVisualSpellCheckingEnabled());
        settingsManager->sync();
        
        qDebug() << "Spell checking enabled for language:" << spellChecker->getCurrentLanguage();
    } else {
        // Update status label
        ui->spellCheckerStatus->setText("Spell checker: Not available");
        ui->spellCheckerStatus->setStyleSheet("color: red; font-size: 10pt;");
        
        qDebug() << "Spell checking not available on this platform";
    }
}

QString MainWindow::mapLanguageToSpellCheckLanguage(const QString &language) const
{
    // Map common language names to spell checker language codes
    static const QMap<QString, QString> languageMap = {
        {"English", "en_US"},
        {"Danish", "da_DK"},
        {"German", "de_DE"},
        {"French", "fr_FR"},
        {"Spanish", "es_ES"},
        {"Italian", "it_IT"},
        {"Portuguese", "pt_PT"},
        {"Dutch", "nl_NL"},
        {"Swedish", "sv_SE"},
        {"Norwegian", "no_NO"},
        {"Finnish", "fi_FI"},
        {"Russian", "ru_RU"},
        {"Polish", "pl_PL"},
        {"Czech", "cs_CZ"},
        {"Hungarian", "hu_HU"},
        {"Turkish", "tr_TR"},
        {"Japanese", "ja_JP"},
        {"Korean", "ko_KR"},
        {"Chinese", "zh_CN"}
    };
    
    return languageMap.value(language, "en_US"); // Default to English if not found
}

void MainWindow::updateSpellCheckerStatusLabel() {
    const QString lang = spellChecker->getCurrentLanguage();
    const bool visual = spellChecker->isVisualSpellCheckingEnabled();
    ui->spellCheckerStatus->setText(QString("Spell checker: %1").arg(lang));
    ui->spellCheckerStatus->setStyleSheet(visual ? "color: green; font-size: 10pt;" : "color: gray; font-size: 10pt;");
}

void MainWindow::editModelSetting(const QString &currentValue, const QString &dialogTitle, 
                                 void (SettingsManager::*setter)(const QString &))
{
    const QString newValue = QInputDialog::getText(this, dialogTitle, 
                                           "Enter the model name:", 
                                           QLineEdit::Normal, currentValue);
    if (!newValue.isEmpty() && newValue != currentValue) {
        (settingsManager->*setter)(newValue);
        settingsManager->sync();
    }
}

void MainWindow::editPromptSetting(PromptType promptType, const QString &currentPrompt,
                                  void (SettingsManager::*setter)(const QString &))
{
    PromptEditDialog dialog(promptType, this);
    dialog.setPrompt(currentPrompt);
    
    if (dialog.exec() == QDialog::Accepted) {
        const QString newPrompt = dialog.getPrompt();
        if (!newPrompt.isEmpty()) {
            (settingsManager->*setter)(newPrompt);
            settingsManager->sync();
        }
    }
}
