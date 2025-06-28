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
#include "NetworkRequestManager.h"

#include <QMainWindow>
#include <QSettings>
#include <QDialog>
#include <QDate>
#include <QLocale>
#include <functional>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

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
    NetworkRequestManager *networkManager;
    
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
    void requestQuickFeedback(const QString &inputText, const QString &sourceLang);
    
    // Utility Methods
    QString mapLanguageToSpellCheckLanguage(const QString &language) const;
    
    // Helper Methods for Settings Actions
    void editModelSetting(const QString &currentValue, const QString &dialogTitle, 
                         void (SettingsManager::*setter)(const QString &));
    void editPromptSetting(PromptType promptType, const QString &currentPrompt,
                          void (SettingsManager::*setter)(const QString &));
};

#endif // MAINWINDOW_H
