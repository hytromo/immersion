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
        m_textChecker = [NSTextChecker sharedTextChecker];
        qDebug() << "macOS spell checker initialized successfully";
    } else {
        qDebug() << "Failed to initialize macOS spell checker";
    }
}

void SpellChecker::cleanupNativeSpellChecker()
{
    m_spellChecker = nullptr;
    m_textChecker = nullptr;
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
    NSArray *guesses = [spellChecker guessesForWordRange:NSMakeRange(0, [nsWord length]) 
                                                 inString:nsWord 
                                                 language:m_currentLanguage.toNSString()];
    
    for (NSString *guess in guesses) {
        suggestions.append(QString::fromNSString(guess));
    }
    
    return suggestions;
}

bool SpellChecker::isWordMisspelled(const QString &word) const
{
    if (!m_spellChecker) return false;
    
    NSSpellChecker *spellChecker = static_cast<NSSpellChecker*>(m_spellChecker);
    NSString *nsWord = word.toNSString();
    NSRange misspelledRange = [spellChecker checkSpellingOfString:nsWord 
                                                       startingAt:0 
                                                          language:m_currentLanguage.toNSString()];
    
    return misspelledRange.location != NSNotFound;
}

void SpellChecker::addWordToDictionary(const QString &word)
{
    if (!m_spellChecker) return;
    
    NSSpellChecker *spellChecker = static_cast<NSSpellChecker*>(m_spellChecker);
    NSString *nsWord = word.toNSString();
    [spellChecker learnWord:nsWord];
}

void SpellChecker::ignoreWordInDocument(const QString &word)
{
    if (!m_spellChecker) return;
    
    NSSpellChecker *spellChecker = static_cast<NSSpellChecker*>(m_spellChecker);
    NSString *nsWord = word.toNSString();
    [spellChecker ignoreWord:nsWord inSpellDocumentWithTag:0];
}

#endif // Q_OS_MACOS 