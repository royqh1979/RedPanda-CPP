#include "customfileiconprovider.h"
#include "iconsmanager.h"
#include "vcs/gitmanager.h"

CustomFileIconProvider::CustomFileIconProvider()
{
    mVCSManager = new GitManager();
}

CustomFileIconProvider::~CustomFileIconProvider()
{
    delete mVCSManager;
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
    if (info.isDir()) {
        if (mVCSManager && mVCSManager->isFileInRepository(info)) {
            if (mVCSManager->isFileInStaged(info))
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_FOLDER_VCS_STAGED);
            else if (mVCSManager->isFileChanged(info))
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_FOLDER_VCS_CHANGED);
            else
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_FOLDER_VCS_NOCHANGE);
        } else
            icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_FOLDER);
    } if (isHFile(info.fileName())) {
        if (mVCSManager && mVCSManager->isFileInRepository(info)) {
            if (mVCSManager->isFileInStaged(info))
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_HFILE_VCS_STAGED);
            else if (mVCSManager->isFileChanged(info))
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_HFILE_VCS_CHANGED);
            else
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_HFILE_VCS_NOCHANGE);
        } else
            icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_HFILE);
    } else if (isCppFile(info.fileName())) {
        if (mVCSManager && mVCSManager->isFileInRepository(info)) {
            if (mVCSManager->isFileInStaged(info))
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_CPPFILE_VCS_STAGED);
            else if (mVCSManager->isFileChanged(info))
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_CPPFILE_VCS_CHANGED);
            else
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_CPPFILE_VCS_NOCHANGE);
        } else
            icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_CPPFILE);
    } else if (isCFile(info.fileName())) {
        if (mVCSManager && mVCSManager->isFileInRepository(info)) {
            if (mVCSManager->isFileInStaged(info))
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_CFILE_VCS_STAGED);
            else if (mVCSManager->isFileChanged(info))
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_CFILE_VCS_CHANGED);
            else
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_CFILE_VCS_NOCHANGE);
        } else
            icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_CFILE);
    } else if (info.suffix()=="dev") {
        if (mVCSManager && mVCSManager->isFileInRepository(info)) {
            if (mVCSManager->isFileInStaged(info))
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_PROJECTFILE_VCS_STAGED);
            else if (mVCSManager->isFileChanged(info))
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_PROJECTFILE_VCS_CHANGED);
            else
                icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_PROJECTFILE_VCS_NOCHANGE);
        } else
            icon = pIconsManager->getIcon(IconsManager::FILESYSTEM_PROJECTFILE);
    }
    if (!icon.isNull())
        return icon;
    return QFileIconProvider::icon(info);
}
