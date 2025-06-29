#include "SpellChecker.h"
#include <QDebug>
#include <QMenu>
#include <QMessageBox>
#include <QInputDialog>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QRegularExpression>
#include <QApplication>

#ifdef Q_OS_MACOS

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

// Constants
namespace {
    const int SPELL_CHECK_DELAY_MS = 500;
    const int MIN_WORD_LENGTH = 2;
    const QString SPELL_CHECK_UNDERLINE_COLOR = "red";
}

SpellChecker::SpellChecker(QObject *parent)
    : QObject(parent)
    , m_textEdit(nullptr)
    , m_spellCheckTimer(new QTimer(this))
    , m_currentLanguage("en")
    , m_selectedWord("")
    , m_contextMenuPos()
    , m_visualSpellCheckingEnabled(false)
    , m_spellChecker(nullptr)
{
    m_replaceAction.reset(new QAction("Replace...", this));
    m_addToDictAction.reset(new QAction("Add to Dictionary", this));
    m_ignoreAction.reset(new QAction("Ignore", this));
    
    setupContextMenu();
    setupVisualSpellChecking();
    initializeNativeSpellChecker();
}

SpellChecker::~SpellChecker()
{
    disableSpellChecking();
    cleanupNativeSpellChecker();
}

void SpellChecker::enableSpellChecking(QPlainTextEdit *textEdit)
{
    if (m_textEdit == textEdit) {
        return; // Already enabled for this widget
    }
    
    disableSpellChecking();
    
    m_textEdit = textEdit;
    if (m_textEdit) {
        // Enable context menu
        m_textEdit->setContextMenuPolicy(Qt::CustomContextMenu);
        connect(m_textEdit, &QPlainTextEdit::customContextMenuRequested,
                this, &SpellChecker::showContextMenu);
        
        // Connect text changes for visual spell checking
        connect(m_textEdit, &QPlainTextEdit::textChanged,
                this, &SpellChecker::onTextChanged);
        
        // Perform initial spell check
        if (m_visualSpellCheckingEnabled) {
            performSpellCheck();
        }
    }
}

void SpellChecker::disableSpellChecking()
{
    if (m_textEdit) {
        m_textEdit->setContextMenuPolicy(Qt::DefaultContextMenu);
        disconnect(m_textEdit, &QPlainTextEdit::customContextMenuRequested,
                   this, &SpellChecker::showContextMenu);
        disconnect(m_textEdit, &QPlainTextEdit::textChanged,
                   this, &SpellChecker::onTextChanged);
        
        // Clear any existing spell check formatting
        clearSpellCheckFormatting();
        
        m_textEdit = nullptr;
    }
}

void SpellChecker::enableVisualSpellChecking(bool enable)
{
    m_visualSpellCheckingEnabled = enable;
    if (enable && m_textEdit && isSpellCheckingAvailable()) {
        performSpellCheck();
    } else if (!enable) {
        clearSpellCheckFormatting();
    }
}

bool SpellChecker::isVisualSpellCheckingEnabled() const
{
    return m_visualSpellCheckingEnabled;
}

void SpellChecker::setupVisualSpellChecking()
{
    // Configure the misspelled word format (red underline)
    m_misspelledFormat.setUnderlineColor(Qt::red);
    m_misspelledFormat.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
    
    // Configure the spell check timer (debounced spell checking)
    m_spellCheckTimer->setSingleShot(true);
    m_spellCheckTimer->setInterval(SPELL_CHECK_DELAY_MS);
    connect(m_spellCheckTimer.data(), &QTimer::timeout, this, &SpellChecker::performSpellCheck);
}

void SpellChecker::onTextChanged()
{
    if (m_visualSpellCheckingEnabled && isSpellCheckingAvailable()) {
        // Restart the timer to debounce spell checking
        m_spellCheckTimer->start();
    }
}

void SpellChecker::performSpellCheck()
{
    if (!m_textEdit || !m_visualSpellCheckingEnabled || !isSpellCheckingAvailable()) {
        qDebug() << "Spell check skipped - enabled:" << m_visualSpellCheckingEnabled << "available:" << isSpellCheckingAvailable();
        return;
    }
    
    qDebug() << "Performing spell check...";
    highlightMisspelledWords();
}

void SpellChecker::clearSpellCheckFormatting()
{
    if (!m_textEdit) {
        return;
    }
    
    QTextCursor cursor(m_textEdit->document());
    cursor.select(QTextCursor::Document);
    cursor.setCharFormat(QTextCharFormat());
}

void SpellChecker::highlightMisspelledWords()
{
    if (!m_textEdit || !isSpellCheckingAvailable()) {
        return;
    }
    
    QString text = m_textEdit->toPlainText();
    if (text.isEmpty()) {
        return;
    }
    
    QStringList words = extractWords(text);
    
    // Save current cursor position and selection
    QTextCursor savedCursor = m_textEdit->textCursor();
    int savedAnchor = savedCursor.anchor();
    int savedPosition = savedCursor.position();

    // Block signals to avoid recursive textChanged
    bool oldBlock = m_textEdit->blockSignals(true);

    QTextCursor cursor(m_textEdit->document());
    cursor.beginEditBlock();
    // Clear all formatting
    cursor.select(QTextCursor::Document);
    cursor.setCharFormat(QTextCharFormat());
    cursor.endEditBlock();

    // Now highlight only misspelled words
    for (const QString &word : words) {
        QString normalizedWord = word.normalized(QString::NormalizationForm_C);
        bool misspelled = isWordMisspelled(normalizedWord);
        if (misspelled) {
            QTextCursor findCursor(m_textEdit->document());
            while (!findCursor.isNull() && !findCursor.atEnd()) {
                findCursor = m_textEdit->document()->find(normalizedWord, findCursor);
                if (!findCursor.isNull()) {
                    QTextCharFormat fmt = findCursor.charFormat();
                    fmt.setUnderlineColor(Qt::red);
                    fmt.setUnderlineStyle(QTextCharFormat::SpellCheckUnderline);
                    findCursor.setCharFormat(fmt);
                }
            }
        }
    }

    // Restore cursor position and selection
    QTextCursor restoreCursor = m_textEdit->textCursor();
    restoreCursor.setPosition(savedAnchor);
    restoreCursor.setPosition(savedPosition, QTextCursor::KeepAnchor);
    m_textEdit->setTextCursor(restoreCursor);

    // Unblock signals
    m_textEdit->blockSignals(oldBlock);
}

QStringList SpellChecker::extractWords(const QString &text) const
{
    QStringList words;
    QStringList lines = text.split('\n', Qt::SkipEmptyParts);
    
    // Regular expression to match words (Unicode letters and numbers)
    static const QRegularExpression wordRegex(R"(\b[\p{L}\p{M}\p{N}]+\b)");
    
    for (const QString &line : lines) {
        QRegularExpressionMatchIterator matchIterator = wordRegex.globalMatch(line);
        while (matchIterator.hasNext()) {
            QRegularExpressionMatch match = matchIterator.next();
            QString word = match.captured();
            
            // Clean and normalize the word
            QString cleanWord = word.normalized(QString::NormalizationForm_C);
            if (cleanWord.length() >= MIN_WORD_LENGTH) {
                words.append(cleanWord);
            }
        }
    }
    
    return words;
}

void SpellChecker::showContextMenu(const QPoint &pos)
{
    if (!m_textEdit || !isSpellCheckingAvailable()) {
        qDebug() << "Context menu skipped - spell checker not available";
        return;
    }
    
    m_contextMenuPos = pos;
    QString word = getWordAtPosition(pos);
    
    qDebug() << "Context menu for word:" << word;
    
    if (word.isEmpty()) {
        qDebug() << "No word at position";
        return;
    }
    
    // Check if the word is misspelled
    if (!isWordMisspelled(word)) {
        qDebug() << "Word is not misspelled, not showing menu";
        return; // Word is spelled correctly, don't show spell check menu
    }
    
    qDebug() << "Showing spell check context menu for misspelled word:" << word;
    
    m_selectedWord = word;
    updateContextMenu();
    
    QMenu contextMenu(m_textEdit);
    
    // Add suggestions
    QStringList suggestions = getSuggestionsForWord(word);
    
    qDebug() << "Suggestions for" << word << ":" << suggestions;
    
    if (!suggestions.isEmpty()) {
        for (const QString &suggestion : suggestions) {
            QAction *action = contextMenu.addAction(suggestion);
            connect(action, &QAction::triggered, [this, suggestion]() {
                replaceWordWithSuggestion(suggestion);
            });
        }
        contextMenu.addSeparator();
    }
    
    // Add spell check actions
    contextMenu.addAction(m_replaceAction.data());
    contextMenu.addAction(m_addToDictAction.data());
    contextMenu.addAction(m_ignoreAction.data());
    
    contextMenu.exec(m_textEdit->mapToGlobal(pos));
}

void SpellChecker::replaceWord()
{
    if (m_selectedWord.isEmpty() || !m_textEdit) {
        return;
    }
    
    QStringList suggestions = getSuggestionsForWord(m_selectedWord);
    
    QString replacement;
    if (!suggestions.isEmpty()) {
        bool ok;
        replacement = QInputDialog::getItem(m_textEdit, "Replace Word",
                                          "Choose replacement:", suggestions, 0, false, &ok);
        if (!ok) {
            return;
        }
    } else {
        bool ok;
        replacement = QInputDialog::getText(m_textEdit, "Replace Word",
                                          "Enter replacement:", QLineEdit::Normal,
                                          m_selectedWord, &ok);
        if (!ok) {
            return;
        }
    }
    
    replaceWordWithSuggestion(replacement);
}

void SpellChecker::addToDictionary()
{
    if (m_selectedWord.isEmpty()) {
        return;
    }
    
    addWordToDictionary(m_selectedWord);
    
    QMessageBox::information(m_textEdit, "Word Added",
                           QString("'%1' has been added to the dictionary.").arg(m_selectedWord));
}

void SpellChecker::ignoreWord()
{
    if (m_selectedWord.isEmpty()) {
        return;
    }
    
    ignoreWordInDocument(m_selectedWord);
    
    QMessageBox::information(m_textEdit, "Word Ignored",
                           QString("'%1' will be ignored in this document.").arg(m_selectedWord));
}

void SpellChecker::replaceWordWithSuggestion(const QString &replacement)
{
    if (!m_textEdit || m_selectedWord.isEmpty()) {
        return;
    }
    
    QTextCursor cursor = getWordCursorAtPosition(m_contextMenuPos);
    if (!cursor.isNull()) {
        cursor.insertText(replacement);
    }
}

QString SpellChecker::getWordAtPosition(const QPoint &pos) const
{
    if (!m_textEdit) {
        return QString();
    }
    
    QTextCursor cursor = getWordCursorAtPosition(pos);
    if (cursor.isNull()) {
        return QString();
    }
    
    return cursor.selectedText();
}

QTextCursor SpellChecker::getWordCursorAtPosition(const QPoint &pos) const
{
    if (!m_textEdit) {
        return QTextCursor();
    }
    
    QTextCursor cursor = m_textEdit->cursorForPosition(pos);
    cursor.select(QTextCursor::WordUnderCursor);
    
    return cursor;
}

void SpellChecker::setupContextMenu()
{
    connect(m_replaceAction.data(), &QAction::triggered, this, &SpellChecker::replaceWord);
    connect(m_addToDictAction.data(), &QAction::triggered, this, &SpellChecker::addToDictionary);
    connect(m_ignoreAction.data(), &QAction::triggered, this, &SpellChecker::ignoreWord);
}

void SpellChecker::updateContextMenu()
{
    if (m_selectedWord.isEmpty()) {
        m_replaceAction->setEnabled(false);
        m_addToDictAction->setEnabled(false);
        m_ignoreAction->setEnabled(false);
    } else {
        m_replaceAction->setEnabled(true);
        m_addToDictAction->setEnabled(true);
        m_ignoreAction->setEnabled(true);
        
        m_replaceAction->setText(QString("Replace '%1'...").arg(m_selectedWord));
        m_addToDictAction->setText(QString("Add '%1' to Dictionary").arg(m_selectedWord));
        m_ignoreAction->setText(QString("Ignore '%1'").arg(m_selectedWord));
    }
}

void SpellChecker::initializeNativeSpellChecker()
{
    // Initialize macOS spell checker
    NSSpellChecker *spellChecker = [NSSpellChecker sharedSpellChecker];
    if (spellChecker) {
        m_spellChecker = spellChecker;
        qDebug() << "macOS spell checker initialized successfully";
        
        // Get the default language
        NSString *defaultLanguage = [spellChecker language];
        if (defaultLanguage) {
            m_currentLanguage = QString::fromNSString(defaultLanguage);
            qDebug() << "Default spell checker language:" << m_currentLanguage;
        }
        
        // Get available languages for debugging
        NSArray *availableLanguages = [spellChecker availableLanguages];
        if (availableLanguages) {
            QStringList languages;
            for (NSString *language in availableLanguages) {
                languages.append(QString::fromNSString(language));
            }
            qDebug() << "Available macOS spell checker languages:" << languages;
        }
    } else {
        qDebug() << "Failed to initialize macOS spell checker";
    }
}

void SpellChecker::cleanupNativeSpellChecker()
{
    m_spellChecker = nullptr;
}

bool SpellChecker::isSpellCheckingAvailable() const
{
    return m_spellChecker != nullptr;
}

QStringList SpellChecker::getAvailableLanguages() const
{
    QStringList languages;
    if (!m_spellChecker) return languages;
    
    NSSpellChecker *spellChecker = static_cast<NSSpellChecker*>(m_spellChecker);
    NSArray *availableLanguages = [spellChecker availableLanguages];
    
    for (NSString *language in availableLanguages) {
        languages.append(QString::fromNSString(language));
    }
    
    return languages;
}

void SpellChecker::setLanguage(const QString &language)
{
    if (!m_spellChecker) return;
    
    NSSpellChecker *spellChecker = static_cast<NSSpellChecker*>(m_spellChecker);
    NSString *nsLanguage = language.toNSString();
    
    if ([spellChecker setLanguage:nsLanguage]) {
        m_currentLanguage = language;
        qDebug() << "Spell checker language set to:" << language;
        
        // Re-perform spell check with new language
        if (m_visualSpellCheckingEnabled && m_textEdit) {
            performSpellCheck();
        }
    } else {
        qDebug() << "Failed to set spell checker language to:" << language;
    }
}

QString SpellChecker::getCurrentLanguage() const
{
    return m_currentLanguage;
}

QStringList SpellChecker::getSuggestionsForWord(const QString &word) const
{
    QStringList suggestions;
    if (!m_spellChecker) return suggestions;
    
    NSSpellChecker *spellChecker = static_cast<NSSpellChecker*>(m_spellChecker);
    NSString *nsWord = word.toNSString();
    
    // Use the modern API for getting suggestions
    NSArray *guesses = [spellChecker guessesForWordRange:NSMakeRange(0, [nsWord length]) 
                                                 inString:nsWord 
                                                 language:nil 
                                 inSpellDocumentWithTag:0];
    
    for (NSString *guess in guesses) {
        suggestions.append(QString::fromNSString(guess));
    }
    
    qDebug() << "macOS suggestions for" << word << ":" << suggestions;
    return suggestions;
}

bool SpellChecker::isWordMisspelled(const QString &word) const
{
    if (!m_spellChecker) return false;
    
    NSSpellChecker *spellChecker = static_cast<NSSpellChecker*>(m_spellChecker);
    NSString *nsWord = word.toNSString();
    
    // Use the standard API for checking spelling
    NSRange misspelledRange = [spellChecker checkSpellingOfString:nsWord 
                                                       startingAt:0];
    
    bool isMisspelled = (misspelledRange.location != NSNotFound);
    qDebug() << "macOS spell check for" << word << ":" << (isMisspelled ? "misspelled" : "correct");
    return isMisspelled;
}

void SpellChecker::addWordToDictionary(const QString &word)
{
    if (!m_spellChecker) return;
    
    NSSpellChecker *spellChecker = static_cast<NSSpellChecker*>(m_spellChecker);
    NSString *nsWord = word.toNSString();
    
    [spellChecker learnWord:nsWord];
    qDebug() << "macOS: Added word to dictionary:" << word;
    
    // Re-perform spell check to update visual indicators
    if (m_visualSpellCheckingEnabled && m_textEdit) {
        performSpellCheck();
    }
}

void SpellChecker::ignoreWordInDocument(const QString &word)
{
    if (!m_spellChecker) return;
    
    NSSpellChecker *spellChecker = static_cast<NSSpellChecker*>(m_spellChecker);
    NSString *nsWord = word.toNSString();
    
    [spellChecker ignoreWord:nsWord inSpellDocumentWithTag:0];
    qDebug() << "macOS: Ignored word in document:" << word;
    
    // Re-perform spell check to update visual indicators
    if (m_visualSpellCheckingEnabled && m_textEdit) {
        performSpellCheck();
    }
}

#endif // Q_OS_MACOS 