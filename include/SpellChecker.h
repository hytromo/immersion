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
    ~SpellChecker();

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
    QPlainTextEdit *m_textEdit;
    QString m_currentLanguage;
    QString m_selectedWord;
    QPoint m_contextMenuPos;
    bool m_visualSpellCheckingEnabled;
    QTimer *m_spellCheckTimer;
    QTextCharFormat m_misspelledFormat;
    
    // Context menu actions
    QAction *m_replaceAction;
    QAction *m_addToDictAction;
    QAction *m_ignoreAction;
    
    // Native macOS spell checker interface
#ifdef Q_OS_MACOS
    void *m_spellChecker; // NSSpellChecker*
    void *m_textChecker;  // NSTextChecker*
#endif
    // Native Windows spell checker interface
#ifdef Q_OS_WIN
    void *m_spellCheckerFactory; // ISpellCheckerFactory*
    void *m_spellChecker;        // ISpellChecker*
#endif
    
    void setupContextMenu();
    void updateContextMenu();
    QString getWordAtPosition(const QPoint &pos);
    QTextCursor getWordCursorAtPosition(const QPoint &pos);
    void replaceWordWithSuggestion(const QString &replacement);
    void setupVisualSpellChecking();
    void clearSpellCheckFormatting();
    void highlightMisspelledWords();
    QStringList extractWords(const QString &text);

    // Native spell checker methods (always declared, only defined for platform or as stubs)
    void initializeNativeSpellChecker();
    void cleanupNativeSpellChecker();
    QStringList getSuggestionsForWord(const QString &word);
    bool isWordMisspelled(const QString &word);
    void addWordToDictionary(const QString &word);
    void ignoreWordInDocument(const QString &word);
};

#endif // SPELLCHECKER_H 