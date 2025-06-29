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
#include "AppConfig.h"

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
#include <QScreen>
#include <QScopedPointer>
#include <QSharedPointer>
#include <memory>

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

MainWindow::~MainWindow() = default;

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
    // Keyboard shortcuts - using smart pointer for better memory management
    goButtonShortcut.reset(new QShortcut(QKeySequence(Qt::CTRL | Qt::Key_Return), this));
    goButtonShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(goButtonShortcut.data(), &QShortcut::activated, this, &MainWindow::on_goButton_clicked);

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
    connect(keychain.data(), &KeychainManager::keyRestored, this,
            [this](const QString &, const QString &value) {
                openaiApiKey = value;
            });
    connect(keychain.data(), &KeychainManager::error, this,
            [this](const QString &) {
                requestApiKeyPopup();
            });
    keychain->readKey(AppConfig::OPENAI_API_KEY_KEYCHAIN_KEY);
}

void MainWindow::requestApiKeyPopup()
{
    ApiKeyDialog dialog(this);
    if (dialog.exec() == QDialog::Accepted) {
        openaiApiKey = dialog.getApiKey();
        if (!openaiApiKey.isEmpty()) {
            keychain->writeKey(AppConfig::OPENAI_API_KEY_KEYCHAIN_KEY, openaiApiKey);
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
    keychain->deleteKey(AppConfig::OPENAI_API_KEY_KEYCHAIN_KEY);
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

bool MainWindow::validateApiKey() const
{
    if (openaiApiKey.isEmpty()) {
        QMessageBox::warning(const_cast<MainWindow*>(this), "Error", "OpenAI API key is missing.");
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
    
    // Add message to history
    addMessageToHistory(inputText);
    
    // Using shared pointer for better memory management - object will be deleted when last reference is gone
    QSharedPointer<OpenAICommunicator> openaiCommunicator(new OpenAICommunicator(openaiApiKey, this));
    openaiCommunicator->setModelName(settingsManager->translationModelName());
    openaiCommunicator->setPromptWithTemplate(settingsManager->translationPrompt(), sourceLang, targetLang, inputText);
    openaiCommunicator->sendRequest();
    
    connect(openaiCommunicator.data(), &OpenAICommunicator::replyReceived, this, 
            [this, inputText, openaiCommunicator](const QString &translation) {
                handleTranslationResponse(translation, inputText);
            });
    
    connect(openaiCommunicator.data(), &OpenAICommunicator::errorOccurred, this, 
            [this, openaiCommunicator](const QString &errorString) {
                handleTranslationError(errorString);
            });
}

void MainWindow::handleTranslationResponse(const QString &translation, const QString &inputText)
{
    auto clipboard = QGuiApplication::clipboard();
    clipboard->setText(translation);
    appDataManager->writeTranslationLog(ui->inputText->toPlainText());
    
    // If quick feedback is enabled, request feedback
    if (ui->quickFeedbackCheckBox->isChecked()) {
        requestQuickFeedback(inputText, ui->sourceLang->text());
    } else {
        ui->goButton->setEnabled(true);
        hide();
    }
}

void MainWindow::handleTranslationError(const QString &errorString)
{
    ui->goButton->setEnabled(true);
    QMessageBox::warning(this, "Network Error", errorString);
}

void MainWindow::requestQuickFeedback(const QString &inputText, const QString &sourceLang)
{
    // Using shared pointer for better memory management - object will be deleted when last reference is gone
    QSharedPointer<OpenAICommunicator> feedbackCommunicator(new OpenAICommunicator(openaiApiKey, this));
    feedbackCommunicator->setModelName(settingsManager->feedbackModelName());
    
    QString feedbackPromptTemplate = settingsManager->feedbackPrompt();
    const QString feedbackPrompt = feedbackPromptTemplate.replace("%sourceLang", sourceLang);
    feedbackCommunicator->setPromptRaw(feedbackPrompt + "\n\n" + inputText);
    feedbackCommunicator->sendRequest();
    
    connect(feedbackCommunicator.data(), &OpenAICommunicator::replyReceived, this, 
            [this, feedbackCommunicator](const QString &feedback) {
                handleFeedbackResponse(feedback);
            });
    
    connect(feedbackCommunicator.data(), &OpenAICommunicator::errorOccurred, this, 
            [this, feedbackCommunicator](const QString &errorString) {
                handleFeedbackError(errorString);
            });
}

void MainWindow::handleFeedbackResponse(const QString &feedback)
{
    ui->goButton->setEnabled(true);
    FeedbackDialog dialog(feedback, this);
    dialog.exec();
    hide();
}

void MainWindow::handleFeedbackError(const QString &errorString)
{
    ui->goButton->setEnabled(true);
    QMessageBox::warning(this, "Feedback Error", "Failed to get feedback: " + errorString);
    hide();
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
    const int currentIndex = availableLanguages.indexOf(currentLanguage);
    const int defaultIndex = (currentIndex == -1) ? 0 : currentIndex;
    
    bool ok;
    const QString newLanguage = QInputDialog::getItem(this, "Edit Spell Checker Language",
                                              "Choose spell checker language:", availableLanguages,
                                              defaultIndex, false, &ok);
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
    const bool newState = !currentlyEnabled;
    spellChecker->enableVisualSpellChecking(newState);
    settingsManager->setVisualSpellCheckingEnabled(newState);
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
        // Add a disabled "No history" action - using std::unique_ptr for better memory management
        std::unique_ptr<QAction> noHistoryAction(new QAction("No history", this));
        noHistoryAction->setEnabled(false);
        ui->menuHistory->addAction(noHistoryAction.release()); // Transfer ownership to menu
    } else {
        // Add history actions - using std::unique_ptr for better memory management
        for (const QString &message : history) {
            // Truncate long messages for display
            const QString displayText = StringUtils::truncateWithEllipsis(message, AppConfig::MAX_HISTORY_DISPLAY_LENGTH);
            
            std::unique_ptr<QAction> historyAction(new QAction(displayText, this));
            historyAction->setData(message); // Store the full message
            historyAction->setToolTip(message); // Show full message in tooltip
            
            connect(historyAction.get(), &QAction::triggered, this, &MainWindow::onHistoryActionTriggered);
            ui->menuHistory->addAction(historyAction.release()); // Transfer ownership to menu
        }
    }
}

void MainWindow::addMessageToHistory(const QString &message)
{
    if (!StringUtils::isEmptyOrWhitespace(message)) {
        settingsManager->addMessageToHistory(message);
        setupHistoryMenu(); // Refresh the menu
    }
}

void MainWindow::onHistoryActionTriggered()
{
    if (auto action = qobject_cast<QAction*>(sender())) {
        const QString message = action->data().toString();
        if (!message.isEmpty()) {
            ui->inputText->setPlainText(message);
            ui->inputText->selectAll(); // Select all text for easy replacement
        }
    }
}

void MainWindow::onGenerateReportActionTriggered()
{
    if (auto action = qobject_cast<QAction*>(sender())) {
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
    
    if (!appDataDir.exists()) {
        std::unique_ptr<QAction> noDataAction(new QAction("No data directory available", this));
        noDataAction->setEnabled(false);
        ui->menuGenerateReport->addAction(noDataAction.release()); // Transfer ownership to menu
        return;
    }
    
    // Get all .txt files in the app data directory
    const QStringList filters{QString("*%1").arg(AppConfig::LOG_FILE_EXTENSION)};
    const QFileInfoList files = appDataDir.entryInfoList(filters, QDir::Files, QDir::Time);
    
    // Create a list of available dates (last 10 days)
    QList<QDate> availableDates;
    const QDate today = QDate::currentDate();
    
    // Check the last 10 days
    for (int i = 0; i < 10; ++i) {
        const QDate checkDate = today.addDays(-i);
        const QString expectedFileName = checkDate.toString("yyyy-MM-dd") + AppConfig::LOG_FILE_EXTENSION;
        
        // Check if file exists
        const bool fileExists = std::any_of(files.begin(), files.end(),
            [&expectedFileName](const QFileInfo &fileInfo) {
                return fileInfo.fileName() == expectedFileName;
            });
        
        if (fileExists) {
            availableDates.append(checkDate);
        }
    }
    
    if (availableDates.isEmpty()) {
        // Add a disabled "No data available" action - using std::unique_ptr for better memory management
        std::unique_ptr<QAction> noDataAction(new QAction("No data available", this));
        noDataAction->setEnabled(false);
        ui->menuGenerateReport->addAction(noDataAction.release()); // Transfer ownership to menu
    } else {
        // Add actions for each available date - using std::unique_ptr for better memory management
        for (const QDate &date : availableDates) {
            const QString displayText = StringUtils::formatDateWithOrdinal(date);
            std::unique_ptr<QAction> reportAction(new QAction(displayText, this));
            reportAction->setData(date.toString("yyyy-MM-dd")); // Store the date as data
            
            connect(reportAction.get(), &QAction::triggered, this, &MainWindow::onGenerateReportActionTriggered);
            ui->menuGenerateReport->addAction(reportAction.release()); // Transfer ownership to menu
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
        // Load saved settings first
        const QString savedSpellLang = settingsManager->spellCheckerLanguage();
        const bool savedVisual = settingsManager->visualSpellCheckingEnabled();
        
        // Set the language BEFORE enabling spell checking for the text edit
        qDebug() << "saved spelling language is" << savedSpellLang;
        const QString sourceLang = ui->sourceLang->text();
        const QString finalLanguage = savedSpellLang.isEmpty() ? mapLanguageToSpellCheckLanguage(sourceLang) : savedSpellLang;
        spellChecker->setLanguage(finalLanguage);
        
        // Now enable spell checking for the input text area (after language is set)
        spellChecker->enableSpellChecking(ui->inputText);
        
        // Enable visual spell checking (red underlines)
        spellChecker->enableVisualSpellChecking(savedVisual);
        
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
        (settingsManager.data()->*setter)(newValue);
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
            (settingsManager.data()->*setter)(newPrompt);
            settingsManager->sync();
        }
    }
}

void MainWindow::centerWindowOnScreen()
{
    if (const QScreen *currentScreen = QGuiApplication::primaryScreen()) {
        const QRect screenGeometry = currentScreen->geometry();
        const QRect windowGeometry = geometry();
        
        const int newX = screenGeometry.center().x() - windowGeometry.width() / 2;
        const int newY = screenGeometry.center().y() - windowGeometry.height() / 2;
        
        // Ensure window doesn't go off-screen
        const int finalX = qMax(screenGeometry.left(), qMin(newX, screenGeometry.right() - windowGeometry.width()));
        const int finalY = qMax(screenGeometry.top(), qMin(newY, screenGeometry.bottom() - windowGeometry.height()));
        
        move(finalX, finalY);
    }
}

bool MainWindow::isWindowOnScreen() const
{
    if (const QScreen *currentScreen = QGuiApplication::primaryScreen()) {
        const QRect screenGeometry = currentScreen->geometry();
        const QRect windowGeometry = geometry();
        return screenGeometry.intersects(windowGeometry);
    }
    return false;
}
