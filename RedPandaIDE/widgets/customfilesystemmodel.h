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
private slots:
    void onRootPathChanged(const QString& folder);
private:
    GitRepository *mGitRepository;
    GitManager *mGitManager;
};

#endif // CUSTOMFILESYSTEMMODEL_H
