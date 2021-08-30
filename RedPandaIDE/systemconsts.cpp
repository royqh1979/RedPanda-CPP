#include "systemconsts.h"
#include <QObject>
#include <QString>
#include <QStringList>

SystemConsts* pSystemConsts;

SystemConsts::SystemConsts(): mDefaultFileFilters()
{
    addDefaultFileFilter(QObject::tr("C files"),"*.c");
    addDefaultFileFilter(QObject::tr("C++ files"),"*.cpp *.cc *.cxx");
    addDefaultFileFilter(QObject::tr("Header files"),"*.h *.hh");
    addDefaultFileFilter(QObject::tr("Text files"),"*.txt");
    addDefaultFileFilter(QObject::tr("All files"),"*");
}

const QStringList &SystemConsts::defaultFileFilters() const noexcept
{
    return mDefaultFileFilters;
}

const QString &SystemConsts::defaultCFileFilter() const noexcept
{
    return mDefaultFileFilters[0];
}

const QString &SystemConsts::defaultCPPFileFilter() const noexcept
{
    return mDefaultFileFilters[1];
}

void SystemConsts::addDefaultFileFilter(const QString &name, const QString &fileExtensions)
{
    mDefaultFileFilters.append(name+ " (" + fileExtensions+")");
}
