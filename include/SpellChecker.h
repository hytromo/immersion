#ifndef SPELLCHECKER_H
#define SPELLCHECKER_H

#include <QObject>
#include <QPlainTextEdit>
#include <QTextCursor>
#include <QMenu>
#include <QAction>
#include <QStringList>
#include <QTimer>
#include <QTextCharFormat>

class SpellChecker : public QObject
{
    Q_OBJECT

public:
    explicit SpellChecker(QObject *parent = nullptr);
    ~SpellChecker() override;

    // Enable spell checking for a text edit widget
    void enableSpellChecking(QPlainTextEdit *textEdit);
    
    // Disable spell checking
    void disableSpellChecking();
    
    // Enable/disable visual spell checking (red underlines)
    void enableVisualSpellChecking(bool enable = true);
    bool isVisualSpellCheckingEnabled() const;
    
    // Check if spell checking is available
    bool isSpellCheckingAvailable() const;
    
    // Get available languages
    QStringList getAvailableLanguages() const;
    
    // Set the language for spell checking
    void setLanguage(const QString &language);
    
    // Get current language
    QString getCurrentLanguage() const;

private slots:
    void showContextMenu(const QPoint &pos);
    void replaceWord();
    void addToDictionary();
    void ignoreWord();
    void onTextChanged();
    void performSpellCheck();

private:
    // UI Components
    QPlainTextEdit *m_textEdit;
    QTimer *m_spellCheckTimer;
    QTextCharFormat m_misspelledFormat;
    
    // Context menu actions
    QAction *m_replaceAction;
    QAction *m_addToDictAction;
    QAction *m_ignoreAction;
    
    // State
    QString m_currentLanguage;
    QString m_selectedWord;
    QPoint m_contextMenuPos;
    bool m_visualSpellCheckingEnabled;
    
    // Native spell checker interfaces
#ifdef Q_OS_MACOS
    void *m_spellChecker; // NSSpellChecker*
    void *m_textChecker;  // NSTextChecker*
#endif
#ifdef Q_OS_WIN
    void *m_spellCheckerFactory; // ISpellCheckerFactory*
    void *m_spellChecker;        // ISpellChecker*
#endif
    
    // Setup Methods
    void setupContextMenu();
    void setupVisualSpellChecking();
    void updateContextMenu();
    
    // Text Processing
    QString getWordAtPosition(const QPoint &pos) const;
    QTextCursor getWordCursorAtPosition(const QPoint &pos) const;
    QStringList extractWords(const QString &text) const;
    
    // Visual Spell Checking
    void clearSpellCheckFormatting();
    void highlightMisspelledWords();
    
    // Word Operations
    void replaceWordWithSuggestion(const QString &replacement);
    
    // Native spell checker methods (platform-specific implementations)
    void initializeNativeSpellChecker();
    void cleanupNativeSpellChecker();
    QStringList getSuggestionsForWord(const QString &word) const;
    bool isWordMisspelled(const QString &word) const;
    void addWordToDictionary(const QString &word);
    void ignoreWordInDocument(const QString &word);
};

#endif // SPELLCHECKER_H 