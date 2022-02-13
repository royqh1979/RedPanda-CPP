#include "customfileiconprovider.h"
#include "iconsmanager.h"

CustomFileIconProvider::CustomFileIconProvider()
{

}

QIcon CustomFileIconProvider::icon(IconType type) const
{
    if (type == IconType::Folder) {
        QIcon icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_FOLDER);
        if (!icon.isNull())
            return icon;
    }
    return QFileIconProvider::icon(type);

}

QIcon CustomFileIconProvider::icon(const QFileInfo &info) const
{
    QIcon icon;
    if (isHFile(info.fileName()))
        icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_HFILE);
    else if (isCppFile(info.fileName())) {
        icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_CPPFILE);
    } else if (isCFile(info.fileName())) {
        icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_CFILE);
    }
    if (!icon.isNull())
        return icon;
    return QFileIconProvider::icon(info);
}
