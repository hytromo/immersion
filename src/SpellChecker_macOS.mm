#include "SpellChecker.h"
#include <QDebug>

#ifdef Q_OS_MACOS

#import <AppKit/AppKit.h>
#import <Foundation/Foundation.h>

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
    
    // Use the correct API for getting suggestions
    NSArray *guesses = [spellChecker guessesForWord:nsWord];
    
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
}

void SpellChecker::ignoreWordInDocument(const QString &word)
{
    if (!m_spellChecker) return;
    
    NSSpellChecker *spellChecker = static_cast<NSSpellChecker*>(m_spellChecker);
    NSString *nsWord = word.toNSString();
    
    [spellChecker ignoreWord:nsWord inSpellDocumentWithTag:0];
    qDebug() << "macOS: Ignored word in document:" << word;
}

#endif // Q_OS_MACOS 