#include "systemconsts.h"
#include <QObject>
#include <QString>
#include <QStringList>

SystemConsts* pSystemConsts;

SystemConsts::SystemConsts(): mDefaultFileFilters()
{
    addDefaultFileFilter(QObject::tr("All files"),"*");
    addDefaultFileFilter(QObject::tr("Dev C++ Project files"),"*.dev");
    addDefaultFileFilter(QObject::tr("C files"),"*.c");
    addDefaultFileFilter(QObject::tr("C++ files"),"*.cpp *.cc *.cxx");
    addDefaultFileFilter(QObject::tr("Header files"),"*.h *.hh");
}

const QStringList &SystemConsts::defaultFileFilters() const noexcept
{
    return mDefaultFileFilters;
}

const QString &SystemConsts::defaultCFileFilter() const noexcept
{
    return mDefaultFileFilters[2];
}

const QString &SystemConsts::defaultCPPFileFilter() const noexcept
{
    return mDefaultFileFilters[3];
}

const QString &SystemConsts::defaultAllFileFilter() const noexcept
{
    return mDefaultFileFilters[0];
}

void SystemConsts::addDefaultFileFilter(const QString &name, const QString &fileExtensions)
{
    mDefaultFileFilters.append(name+ " (" + fileExtensions+")");
}
