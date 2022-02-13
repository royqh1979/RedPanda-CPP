#include "customfilesystemmodel.h"
#include "../vcs/gitmanager.h"
#include "../vcs/gitrepository.h"

CustomFileSystemModel::CustomFileSystemModel(QObject *parent) : QFileSystemModel(parent)
{
    mGitManager = new GitManager(this);
    mGitRepository = new GitRepository("",mGitManager,mGitManager);
    connect(this,&QFileSystemModel::rootPathChanged,
            this, &CustomFileSystemModel::onRootPathChanged);
}

QVariant CustomFileSystemModel::data(const QModelIndex &index, int role) const
{
    return QFileSystemModel::data(index,role);
}

void CustomFileSystemModel::onRootPathChanged(const QString &folder)
{
    mGitRepository->setFolder(folder);
}
