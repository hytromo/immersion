#include "StringUtils.h"
#include <QRegularExpression>

QString StringUtils::formatDateWithOrdinal(const QDate &date)
{
    const int day = date.day();
    const QString daySuffix = getOrdinalSuffix(day);
    const QLocale locale;
    const QString monthName = locale.monthName(date.month(), QLocale::LongFormat);
    const int year = date.year();
    
    return QString("%1%2 of %3 %4").arg(day).arg(daySuffix).arg(monthName).arg(year);
}

QString StringUtils::truncateWithEllipsis(const QString &text, int maxLength)
{
    if (text.length() <= maxLength) {
        return text;
    }
    return text.left(maxLength - 3) + "...";
}

QString StringUtils::cleanText(const QString &text)
{
    return text.trimmed();
}

bool StringUtils::isEmptyOrWhitespace(const QString &text)
{
    return text.trimmed().isEmpty();
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