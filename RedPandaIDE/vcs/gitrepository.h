#ifndef GITREPOSITORY_H
#define GITREPOSITORY_H

#include <QFileInfo>
#include <QObject>
#include <QSet>
#include <memory>
#include "gitutils.h"

class GitManager;
class GitRepository : public QObject
{
    Q_OBJECT
public:
    explicit GitRepository(const QString& folder, QObject *parent = nullptr);
    ~GitRepository();

    const QString &folder() const;

    void createRepository();
    bool hasRepository(QString& currentBranch);

    bool isFileInRepository(const QFileInfo& fileInfo) {
        return isFileInRepository(fileInfo.absoluteFilePath());
    }
    bool isFileInRepository(const QString& filePath) {
        return mFilesInRepositories.contains(filePath);
    }
    bool isFileStaged(const QFileInfo& fileInfo) {
        return isFileStaged(fileInfo.absoluteFilePath());
    }
    bool isFileStaged(const QString& filePath) {
        return mStagedFiles.contains(filePath);
    }
    bool hasStagedFiles() {
        return !mStagedFiles.isEmpty();
    }
    bool isFileChanged(const QFileInfo& fileInfo) {
        return isFileChanged(fileInfo.absoluteFilePath());
    }
    bool isFileChanged(const QString& filePath) {
        return mChangedFiles.contains(filePath);
    }
    bool hasChangedFiles() {
        return !mChangedFiles.isEmpty();
    }

    void add(const QString& path);
    void remove(const QString& path);
    void rename(const QString& oldName, const QString& newName);
    void restore(const QString& path);
    QSet<QString> listFiles(bool refresh);

    void clone(const QString& url);
    void commit(const QString& message, bool autoStage=true);
    void revert();
    void reset(const QString& commit, GitResetStrategy strategy);


    void setFolder(const QString &newFolder);
    void update();

    const QString &realFolder() const;

signals:
private:
    QString mRealFolder;
    QString mFolder;
    bool mInRepository;
    QString mBranch;
    GitManager* mManager;
    QSet<QString> mFilesInRepositories;
    QSet<QString> mChangedFiles;
    QSet<QString> mStagedFiles;
private:
    void convertFilesListToSet(const QStringList& filesList,QSet<QString>& set);
};

#endif // GITREPOSITORY_H
