#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <QString>
#include <QDate>
#include <QRegularExpression>

class StringUtils
{
public:
    // Date formatting
    static QString formatDateWithOrdinal(const QDate &date);
    static QString getOrdinalSuffix(int number);
    
    // Text manipulation
    static QString truncateWithEllipsis(const QString &text, int maxLength);
    static QString cleanText(const QString &text);
    static QString normalizeWhitespace(const QString &text);
    static QString removeExtraSpaces(const QString &text);
    
    // Text validation
    static bool isEmptyOrWhitespace(const QString &text);
    static bool isValidText(const QString &text);
    static bool containsOnlyWhitespace(const QString &text);
    
    // Text analysis
    static int wordCount(const QString &text);
    static int characterCount(const QString &text, bool includeSpaces = true);
    static QString getFirstWord(const QString &text);
    static QString getLastWord(const QString &text);
    
    // Text formatting
    static QString capitalizeFirstLetter(const QString &text);
    static QString toTitleCase(const QString &text);
    static QString removePunctuation(const QString &text);
    
    // Language detection helpers
    static bool containsNonLatinCharacters(const QString &text);
    static bool isLikelyEnglish(const QString &text);
    
    // Constants
    static constexpr int DEFAULT_MAX_LENGTH = 100;
    static constexpr int MIN_VALID_TEXT_LENGTH = 1;
    
private:
    // Private constructor to prevent instantiation
    StringUtils() = delete;
    
    // Helper methods
    static bool isPunctuation(QChar ch);
    static bool isWordBoundary(QChar ch);
};

#endif // STRINGUTILS_H 