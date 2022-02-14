#ifndef CUSTOMFILESYSTEMMODEL_H
#define CUSTOMFILESYSTEMMODEL_H

#include <QFileSystemModel>


class GitRepository;
class GitManager;
class CustomFileSystemModel : public QFileSystemModel
{
    Q_OBJECT
public:
    explicit CustomFileSystemModel(QObject *parent = nullptr);

    // QAbstractItemModel interface
public:
    QVariant data(const QModelIndex &index, int role) const override;
};

#endif // CUSTOMFILESYSTEMMODEL_H
