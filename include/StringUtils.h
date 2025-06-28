#ifndef STRINGUTILS_H
#define STRINGUTILS_H

#include <QString>
#include <QDate>
#include <QLocale>

class StringUtils
{
public:
    // Format date with proper ordinal suffixes
    static QString formatDateWithOrdinal(const QDate &date);
    
    // Truncate text with ellipsis
    static QString truncateWithEllipsis(const QString &text, int maxLength);
    
    // Clean and normalize text
    static QString cleanText(const QString &text);
    
    // Check if string is empty or whitespace only
    static bool isEmptyOrWhitespace(const QString &text);
    
    // Get ordinal suffix for a number
    static QString getOrdinalSuffix(int number);

private:
    StringUtils() = delete; // Prevent instantiation
};

#endif // STRINGUTILS_H 