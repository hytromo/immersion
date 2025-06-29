#include "StringUtils.h"
#include <QRegularExpression>
#include <QLocale>
#include <QChar>
#include <algorithm>

QString StringUtils::formatDateWithOrdinal(const QDate &date)
{
    if (!date.isValid()) {
        return QString();
    }
    
    const int day = date.day();
    const QString daySuffix = getOrdinalSuffix(day);
    const QLocale locale;
    const QString monthName = locale.monthName(date.month(), QLocale::LongFormat);
    const int year = date.year();
    
    return QString("%1%2 of %3 %4").arg(day).arg(daySuffix).arg(monthName).arg(year);
}

QString StringUtils::truncateWithEllipsis(const QString &text, int maxLength)
{
    if (maxLength < 3) {
        return QString();
    }
    
    if (text.length() <= maxLength) {
        return text;
    }
    return text.left(maxLength - 3) + "...";
}

QString StringUtils::cleanText(const QString &text)
{
    return text.trimmed();
}

QString StringUtils::normalizeWhitespace(const QString &text)
{
    if (text.isEmpty()) {
        return text;
    }
    
    // Replace multiple whitespace characters with a single space
    QRegularExpression whitespaceRegex("\\s+");
    QString normalized = text.trimmed();
    normalized.replace(whitespaceRegex, " ");
    
    return normalized;
}

QString StringUtils::removeExtraSpaces(const QString &text)
{
    return normalizeWhitespace(text);
}

bool StringUtils::isEmptyOrWhitespace(const QString &text)
{
    return text.trimmed().isEmpty();
}

bool StringUtils::isValidText(const QString &text)
{
    return !text.isEmpty() && text.length() >= MIN_VALID_TEXT_LENGTH && !containsOnlyWhitespace(text);
}

bool StringUtils::containsOnlyWhitespace(const QString &text)
{
    return std::all_of(text.begin(), text.end(), [](QChar ch) {
        return ch.isSpace();
    });
}

int StringUtils::wordCount(const QString &text)
{
    if (text.isEmpty()) {
        return 0;
    }
    
    const QString normalized = normalizeWhitespace(text);
    if (normalized.isEmpty()) {
        return 0;
    }
    
    return normalized.count(' ') + 1;
}

int StringUtils::characterCount(const QString &text, bool includeSpaces)
{
    if (includeSpaces) {
        return text.length();
    }
    
    return text.count(QRegularExpression("\\S"));
}

QString StringUtils::getFirstWord(const QString &text)
{
    const QString normalized = normalizeWhitespace(text);
    if (normalized.isEmpty()) {
        return QString();
    }
    
    const int spaceIndex = normalized.indexOf(' ');
    return spaceIndex == -1 ? normalized : normalized.left(spaceIndex);
}

QString StringUtils::getLastWord(const QString &text)
{
    const QString normalized = normalizeWhitespace(text);
    if (normalized.isEmpty()) {
        return QString();
    }
    
    const int lastSpaceIndex = normalized.lastIndexOf(' ');
    return lastSpaceIndex == -1 ? normalized : normalized.mid(lastSpaceIndex + 1);
}

QString StringUtils::capitalizeFirstLetter(const QString &text)
{
    if (text.isEmpty()) {
        return text;
    }
    
    QString result = text;
    result[0] = result[0].toUpper();
    return result;
}

QString StringUtils::toTitleCase(const QString &text)
{
    if (text.isEmpty()) {
        return text;
    }
    
    QString result = text.toLower();
    bool capitalizeNext = true;
    
    for (int i = 0; i < result.length(); ++i) {
        if (capitalizeNext && result[i].isLetter()) {
            result[i] = result[i].toUpper();
            capitalizeNext = false;
        } else if (isWordBoundary(result[i])) {
            capitalizeNext = true;
        }
    }
    
    return result;
}

QString StringUtils::removePunctuation(const QString &text)
{
    if (text.isEmpty()) {
        return text;
    }
    
    QString result;
    result.reserve(text.length());
    
    for (QChar ch : text) {
        if (!isPunctuation(ch)) {
            result.append(ch);
        }
    }
    
    return result;
}

bool StringUtils::containsNonLatinCharacters(const QString &text)
{
    return std::any_of(text.begin(), text.end(), [](QChar ch) {
        return ch.unicode() > 127;
    });
}

bool StringUtils::isLikelyEnglish(const QString &text)
{
    if (text.isEmpty()) {
        return false;
    }
    
    // Simple heuristic: if more than 80% of characters are ASCII, it's likely English
    int asciiCount = 0;
    for (QChar ch : text) {
        if (ch.unicode() <= 127) {
            asciiCount++;
        }
    }
    
    return (static_cast<double>(asciiCount) / text.length()) > 0.8;
}

QString StringUtils::getOrdinalSuffix(int number)
{
    // Handle special cases for 11, 12, 13
    if (number >= 11 && number <= 13) {
        return "th";
    }
    
    // Handle other cases based on last digit
    switch (number % 10) {
        case 1: return "st";
        case 2: return "nd";
        case 3: return "rd";
        default: return "th";
    }
}

bool StringUtils::isPunctuation(QChar ch)
{
    static const QString punctuation = ".,;:!?\"'()[]{}<>-_=+@#$%^&*~`|\\/";
    return punctuation.contains(ch);
}

bool StringUtils::isWordBoundary(QChar ch)
{
    return ch.isSpace() || isPunctuation(ch);
} 