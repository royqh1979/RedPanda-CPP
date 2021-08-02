#include "SearchBase.h"

SynSearchBase::SynSearchBase(QObject *parent) : QObject(parent)
{

}

QString SynSearchBase::pattern()
{
    return mPattern;
}

void SynSearchBase::setPattern(const QString &value)
{
    mPattern = value;
}

SynSearchOptions SynSearchBase::options() const
{
    return mOptions;
}

void SynSearchBase::setOptions(const SynSearchOptions &options)
{
    mOptions = options;
}
