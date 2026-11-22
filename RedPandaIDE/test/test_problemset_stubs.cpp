#include <QIcon>
#include "src/iconsmanager.h"

IconsManager::IconsManager(DirSettings *dirSettings, const QString &language, QObject *parent)
    : QObject(parent)
{
    Q_UNUSED(dirSettings)
    Q_UNUSED(language)
}

QIcon IconsManager::getIcon(IconName iconName) const
{
    Q_UNUSED(iconName)
    return QIcon();
}
