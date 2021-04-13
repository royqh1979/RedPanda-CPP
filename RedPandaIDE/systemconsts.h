#ifndef SYSTEMCONSTS_H
#define SYSTEMCONSTS_H

#include <QStringList>

#define APP_SETTSINGS_FILENAME "redpandacpp.ini"

class SystemConsts
{
public:
    SystemConsts();
    const QStringList& defaultFileFilters() const noexcept;
    const QString& defaultFileFilter() const noexcept;
    void addDefaultFileFilter(const QString& name, const QString& fileExtensions);
private:
    QStringList mDefaultFileFilters;
};

extern SystemConsts* pSystemConsts;
#endif // SYSTEMCONSTS_H
