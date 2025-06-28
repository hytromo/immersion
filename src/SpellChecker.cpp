#include "SpellChecker.h"
#include <QApplication>
#include <QDebug>
#include <QInputDialog>
#include <QMessageBox>
#include <QTimer>
#include <QRegularExpression>

#ifdef Q_OS_WIN
#include <windows.h>
#include <objbase.h>
#include <spellcheck.h>
#include <comdef.h>
#ifdef _MSC_VER
#pragma comment(lib, "ole32.lib")
#endif
#endif

// Constants
namespace {
    const int SPELL_CHECK_DELAY_MS = 500;
    const int MIN_WORD_LENGTH = 2;
    const QString SPELL_CHECK_UNDERLINE_COLOR = "red";
}

// Helper function to convert Qt language codes to Windows language tags
QString toWindowsLang(const QString &lang) {
#ifdef Q_OS_WIN
    QString winLang = lang;
    winLang.replace('_', '-');
    if (winLang.isEmpty())
        return "en-US";
    return winLang;
#else
    return lang;
#endif
}

SpellChecker::SpellChecker(QObject *parent)
    : QObject(parent)
    , m_textEdit(nullptr)
    , m_spellCheckTimer(new QTimer(this))
    , m_replaceAction(new QAction("Replace...", this))
    , m_addToDictAction(new QAction("Add to Dictionary", this))
    , m_ignoreAction(new QAction("Ignore", this))
    , m_currentLanguage("en_US")
    , m_selectedWord("")
    , m_contextMenuPos()
    , m_visualSpellCheckingEnabled(false)
#ifdef Q_OS_WIN
    , m_spellCheckerFactory(nullptr)
    , m_spellChecker(nullptr)
#endif
{
    initializeNativeSpellChecker();
    setupContextMenu();
    setupVisualSpellChecking();
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

bool SpellChecker::isSpellCheckingAvailable() const
{
#ifdef Q_OS_WIN
    return m_spellChecker != nullptr;
#else
    return false;
#endif
}

QStringList SpellChecker::getAvailableLanguages() const
{
#ifdef Q_OS_WIN
    QStringList languages;
    if (!m_spellCheckerFactory) return languages;
    IEnumString *langEnum = nullptr;
    HRESULT hr = static_cast<ISpellCheckerFactory*>(m_spellCheckerFactory)->get_SupportedLanguages(&langEnum);
    if (SUCCEEDED(hr) && langEnum) {
        LPOLESTR langStr = nullptr;
        ULONG fetched = 0;
        while (langEnum->Next(1, &langStr, &fetched) == S_OK) {
            languages.append(QString::fromWCharArray(langStr));
            CoTaskMemFree(langStr);
        }
        langEnum->Release();
    }
    return languages;
#else
    return QStringList();
#endif
}

void SpellChecker::setLanguage(const QString &language)
{
    m_currentLanguage = language;
#ifdef Q_OS_WIN
    if (m_spellChecker) {
        static_cast<ISpellChecker*>(m_spellChecker)->Release();
        m_spellChecker = nullptr;
    }
    if (m_spellCheckerFactory) {
        ISpellChecker *checker = nullptr;
        QString lang = toWindowsLang(language);
        qDebug() << "Setting Windows spell checker language to:" << lang;
        
        std::wstring wideLang = lang.toStdWString();
        HRESULT hr = static_cast<ISpellCheckerFactory*>(m_spellCheckerFactory)->CreateSpellChecker(wideLang.c_str(), &checker);
        
        if (SUCCEEDED(hr)) {
            m_spellChecker = checker;
            qDebug() << "Windows spell checker language set to:" << lang;
        } else {
            m_spellChecker = nullptr;
            qWarning() << "Failed to set Windows spell checker language to:" << lang << "HRESULT:" << QString::number(hr, 16);
            
            // Try fallback to en-US
            qDebug() << "Trying fallback to en-US";
            hr = static_cast<ISpellCheckerFactory*>(m_spellCheckerFactory)->CreateSpellChecker(L"en-US", &checker);
            if (SUCCEEDED(hr)) {
                m_spellChecker = checker;
                qDebug() << "Windows spell checker set to fallback en-US";
            } else {
                qWarning() << "Failed to set Windows spell checker even with en-US fallback";
            }
        }
    }
#endif
    
    // Re-perform spell check with new language
    if (m_visualSpellCheckingEnabled) {
        performSpellCheck();
    }
}

QString SpellChecker::getCurrentLanguage() const
{
    return m_currentLanguage;
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

void SpellChecker::initializeNativeSpellChecker()
{
#ifdef Q_OS_WIN
    HRESULT hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr) && hr != RPC_E_CHANGED_MODE) {
        qWarning() << "Failed to initialize COM for spell checker";
        return;
    }
    qDebug() << "COM initialized successfully";
    
    ISpellCheckerFactory *factory = nullptr;
    hr = CoCreateInstance(__uuidof(SpellCheckerFactory), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&factory));
    if (FAILED(hr)) {
        qWarning() << "Failed to create ISpellCheckerFactory, HRESULT:" << QString::number(hr, 16);
        return;
    }
    qDebug() << "ISpellCheckerFactory created successfully";
    m_spellCheckerFactory = factory;
    
    // Get available languages first
    IEnumString *langEnum = nullptr;
    hr = factory->get_SupportedLanguages(&langEnum);
    if (SUCCEEDED(hr) && langEnum) {
        LPOLESTR langStr = nullptr;
        ULONG fetched = 0;
        QStringList availableLangs;
        while (langEnum->Next(1, &langStr, &fetched) == S_OK) {
            availableLangs.append(QString::fromWCharArray(langStr));
            CoTaskMemFree(langStr);
        }
        langEnum->Release();
        qDebug() << "Available Windows spell checker languages:" << availableLangs;
    }
    
    // Try to create spell checker with the desired language
    ISpellChecker *checker = nullptr;
    QString lang = toWindowsLang(m_currentLanguage);
    qDebug() << "Creating spell checker for language:" << lang;
    
    std::wstring wideLang = lang.toStdWString();
    hr = factory->CreateSpellChecker(wideLang.c_str(), &checker);
    
    if (SUCCEEDED(hr)) {
        m_spellChecker = checker;
        qDebug() << "Windows spell checker initialized for language:" << lang;
    } else {
        m_spellChecker = nullptr;
        qWarning() << "Failed to create ISpellChecker for language:" << lang << "HRESULT:" << QString::number(hr, 16);
        
        // Try with en-US as fallback
        qDebug() << "Trying fallback to en-US";
        hr = factory->CreateSpellChecker(L"en-US", &checker);
        if (SUCCEEDED(hr)) {
            m_spellChecker = checker;
            qDebug() << "Windows spell checker initialized with fallback en-US";
        } else {
            qWarning() << "Failed to create ISpellChecker even with en-US fallback, HRESULT:" << QString::number(hr, 16);
        }
    }
#endif
}

void SpellChecker::cleanupNativeSpellChecker()
{
#ifdef Q_OS_WIN
    if (m_spellChecker) {
        static_cast<ISpellChecker*>(m_spellChecker)->Release();
        m_spellChecker = nullptr;
    }
    if (m_spellCheckerFactory) {
        static_cast<ISpellCheckerFactory*>(m_spellCheckerFactory)->Release();
        m_spellCheckerFactory = nullptr;
    }
    CoUninitialize();
#endif
}

QStringList SpellChecker::getSuggestionsForWord(const QString &word) const
{
#ifdef Q_OS_WIN
    QString normWord = word.normalized(QString::NormalizationForm_C);
    if (!m_spellChecker) {
        qDebug() << "Windows spell checker not available for suggestions for word:" << normWord;
        return QStringList();
    }
    
    // Convert word to wide string
    std::wstring wideWord = normWord.toStdWString();
    
    // Get suggestions for the word
    IEnumString *enumSuggestions = nullptr;
    HRESULT hr = static_cast<ISpellChecker*>(m_spellChecker)->Suggest(wideWord.c_str(), &enumSuggestions);
    
    QStringList suggestions;
    if (SUCCEEDED(hr) && enumSuggestions) {
        LPOLESTR suggestion = nullptr;
        ULONG fetched = 0;
        
        while (enumSuggestions->Next(1, &suggestion, &fetched) == S_OK) {
            suggestions.append(QString::fromWCharArray(suggestion));
            CoTaskMemFree(suggestion);
        }
        
        enumSuggestions->Release();
    }
    
    qDebug() << "Windows suggestions for" << normWord << ":" << suggestions;
    return suggestions;
#else
    return QStringList();
#endif
}

bool SpellChecker::isWordMisspelled(const QString &word) const
{
#ifdef Q_OS_WIN
    QString normWord = word.normalized(QString::NormalizationForm_C);
    if (!m_spellChecker) {
        qDebug() << "Windows spell checker not available for word:" << normWord;
        return false;
    }
    
    // Convert word to wide string
    std::wstring wideWord = normWord.toStdWString();
    
    // Check if the word is misspelled
    IEnumSpellingError *errors = nullptr;
    HRESULT hr = static_cast<ISpellChecker*>(m_spellChecker)->Check(wideWord.c_str(), &errors);
    
    if (SUCCEEDED(hr) && errors) {
        ISpellingError *error = nullptr;
        bool hasError = false;
        
        // Check if there are any spelling errors
        if (errors->Next(&error) == S_OK) {
            hasError = true;
            error->Release();
        }
        
        errors->Release();
        qDebug() << "Windows spell check for" << normWord << ":" << (hasError ? "misspelled" : "correct");
        return hasError;
    } else {
        qDebug() << "Windows spell check failed for" << normWord << "HRESULT:" << QString::number(hr, 16);
        return false;
    }
#else
    return false;
#endif
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

void SpellChecker::addWordToDictionary(const QString &word)
{
#ifdef Q_OS_WIN
    if (!m_spellChecker) return;
    static_cast<ISpellChecker*>(m_spellChecker)->Add(_bstr_t(word.toStdWString().c_str()));
#endif
}

void SpellChecker::ignoreWordInDocument(const QString &word)
{
#ifdef Q_OS_WIN
    // Windows spell checker does not have a per-document ignore, so we do nothing or could implement a local ignore list.
    Q_UNUSED(word);
#endif
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