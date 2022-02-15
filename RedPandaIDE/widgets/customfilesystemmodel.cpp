#include "customfilesystemmodel.h"
#include "../vcs/gitmanager.h"
#include "../vcs/gitrepository.h"

CustomFileSystemModel::CustomFileSystemModel(QObject *parent) : QFileSystemModel(parent)
{
}

QVariant CustomFileSystemModel::data(const QModelIndex &index, int role) const
{
    return QFileSystemModel::data(index,role);
}

