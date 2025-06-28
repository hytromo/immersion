#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "KeychainManager.h"
#include "OpenAICommunicator.h"
#include "AppDataManager.h"
#include "SettingsManager.h"
#include "ApiKeyDialog.h"
#include "PromptEditDialog.h"
#include "FeedbackDialog.h"
#include "SpellChecker.h"

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
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;

private slots:
    // UI Event Handlers
    void on_goButton_clicked();
    void onHistoryActionTriggered();
    void onGenerateReportActionTriggered();
    
    // Menu Actions
    void actionReset_OpenAI_API_key();
    void actionOpenCorrectionsFolder();
    void actionGenerateMistakesReport();
    void actionHelp();
    void actionQuit();
    
    // Settings Actions
    void actionEditTranslationModel();
    void actionEditReportsModel();
    void actionEditFeedbackModel();
    void actionEditTranslationPrompt();
    void actionEditReportPrompt();
    void actionEditFeedbackPrompt();
    void actionEditSpellCheckerLanguage();
    void actionToggleVisualSpellChecking();

private:
    // UI Components
    Ui::MainWindow *ui;
    
    // Managers
    KeychainManager *keychain;
    AppDataManager *appDataManager;
    SettingsManager *settingsManager;
    SpellChecker *spellChecker;
    
    // State
    QString openaiApiKey;

    // Constants
    static const QString OPENAI_API_KEY_KEYCHAIN_KEY;
    static const int MAX_HISTORY_SIZE;
    static const int SPELL_CHECK_DELAY_MS;
    static const int MAX_HISTORY_DISPLAY_LENGTH;

    // Initialization Methods
    void initializeUI();
    void setupConnections();
    void setupSpellChecker();
    
    // API Key Management
    void retrieveOpenAIApiKey();
    void requestApiKeyPopup();
    bool validateApiKey();
    
    // UI Setup
    void setupHistoryMenu();
    void setupGenerateReportMenu();
    void updateSpellCheckerStatusLabel();
    
    // Data Management
    void addMessageToHistory(const QString &message);
    void generateReportForDate(const QString &dateString);
    void saveSettings();
    
    // Network Request Helpers
    void setupReportRequest(OpenAICommunicator *communicator, const QString &fileContent, const QString &dateString);
    void connectOpenAICommunicator(OpenAICommunicator *communicator, QDialog *progress, 
                                  std::function<void(const QString&)> successCallback);
    void requestQuickFeedback(const QString &inputText, const QString &sourceLang);
    
    // Utility Methods
    void cleanupProgressAndCommunicator(QDialog *progress, OpenAICommunicator *communicator);
    QString formatDateForDisplay(const QDate &date) const;
    QString mapLanguageToSpellCheckLanguage(const QString &language) const;
    
    // Helper Methods for Settings Actions
    void editModelSetting(const QString &currentValue, const QString &dialogTitle, 
                         void (SettingsManager::*setter)(const QString &));
    void editPromptSetting(PromptType promptType, const QString &currentPrompt,
                          void (SettingsManager::*setter)(const QString &));
};

#endif // MAINWINDOW_H
