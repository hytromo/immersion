#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "KeychainManager.h"
#include "OpenAICommunicator.h"
#include "AppDataManager.h"
#include "SettingsManager.h"
#include "ApiKeyDialog.h"
#include "PromptEditDialog.h"
#include "ModelEditDialog.h"
#include "FeedbackDialog.h"
#include "SpellChecker.h"
#include "AppConfig.h"
#include "ProgressDialog.h"

#include <QMainWindow>
#include <QSettings>
#include <QDialog>
#include <QDate>
#include <QLocale>
#include <QScopedPointer>
#include <QSharedPointer>
#include <QShortcut>
#include <QAction>
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
    
    // Public helper methods for window management
    void centerWindowOnScreen();
    bool isWindowOnScreen() const;

private slots:
    // UI Event Handlers
    void on_goButton_clicked();
    void onHistoryActionTriggered();
    void onGenerateReportActionTriggered();
    void finalizeRequests();
    
    // Menu Actions
    void actionReset_OpenAI_API_key();
    void actionOpenCorrectionsFolder();
    void actionHelp();
    void actionQuit();
    
    // Settings Actions
    void actionEditModels();
    void actionEditPrompts();
    void actionEditSpellCheckerLanguage();
    void actionToggleVisualSpellChecking();

private:
    // UI Components
    QScopedPointer<Ui::MainWindow> ui;
    
    // Managers - using smart pointers for better memory management
    QScopedPointer<KeychainManager> keychain;
    QScopedPointer<AppDataManager> appDataManager;
    QScopedPointer<SettingsManager> settingsManager;
    QScopedPointer<SpellChecker> spellChecker;
    
    // Smart pointers for dynamically created objects
    QScopedPointer<QShortcut> goButtonShortcut;
    
    // State
    QString openaiApiKey;
    
    // Parallel request tracking
    QSharedPointer<OpenAICommunicator> translationCommunicator;
    QSharedPointer<OpenAICommunicator> feedbackCommunicator;
    QSharedPointer<OpenAICommunicator> reportCommunicator;
    bool translationCompleted = false;
    bool feedbackCompleted = false;
    QString pendingTranslation;
    QString pendingFeedback;

    // Initialization Methods
    void initializeUI();
    void setupConnections();
    void setupSpellChecker();
    
    // API Key Management
    void retrieveOpenAIApiKey();
    void requestApiKeyPopup();
    bool validateApiKey() const;
    
    // UI Setup
    void setupHistoryMenu();
    void setupGenerateReportMenu();
    void updateSpellCheckerStatusLabel();
    
    // Data Management
    void addMessageToHistory(const QString &message);
    void generateReportForDate(const QString &dateString);
    void saveSettings();
    
    // Network Request Helpers
    void startParallelRequests(const QString &inputText, const QString &sourceLang, const QString &targetLang);
    void checkBothRequestsCompleted();
    
    // Utility Methods
    QString mapLanguageToSpellCheckLanguage(const QString &language) const;
    
    // Helper Methods for Settings Actions
    void editModelSetting(const QString &currentValue, const QString &dialogTitle, 
                         void (SettingsManager::*setter)(const QString &));

    QScopedPointer<ProgressDialog> progressDialog;
};

#endif // MAINWINDOW_H
