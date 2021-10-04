#include "SearchRegex.h"

#include <QRegExp>

SynSearchRegex::SynSearchRegex(QObject* parent):SynSearchBase(parent)
{

}

int SynSearchRegex::length(int aIndex)
{
    if (aIndex<0 || aIndex >= mResults.length())
        return -1;
    return mLengths[aIndex];
}

int SynSearchRegex::result(int aIndex)
{
    if (aIndex<0 || aIndex >= mResults.length())
        return -1;
    return mResults[aIndex];
}

int SynSearchRegex::resultCount()
{
    return mResults.size();
}

int SynSearchRegex::findAll(const QString &text)
{
    if (pattern().isEmpty())
        return 0;
    mResults.clear();
    mLengths.clear();
    QRegularExpressionMatchIterator it = mRegex.globalMatch(text);
    while (it.hasNext()) {
        QRegularExpressionMatch match = it.next();
        mLengths.append(match.capturedLength());
        mResults.append(match.capturedStart());
    }
    return mResults.size();
}

QString SynSearchRegex::replace(const QString &aOccurrence, const QString &aReplacement)
{
    QString s=aOccurrence;
    return s.replace(mRegex,aReplacement);
}

void SynSearchRegex::setPattern(const QString &value)
{
    SynSearchBase::setPattern(value);
    mRegex.setPattern(value);
    updateRegexOptions();
}

void SynSearchRegex::setOptions(const SynSearchOptions &options)
{
    SynSearchBase::setOptions(options);
    updateRegexOptions();
}

void SynSearchRegex::updateRegexOptions()
{
    if (options().testFlag(SynSearchOption::ssoMatchCase)) {
        mRegex.setPatternOptions(
                    mRegex.patternOptions() &
                    ~QRegularExpression::CaseInsensitiveOption);
    } else {
        mRegex.setPatternOptions(
                    mRegex.patternOptions() |
                    QRegularExpression::CaseInsensitiveOption);
    }
}
