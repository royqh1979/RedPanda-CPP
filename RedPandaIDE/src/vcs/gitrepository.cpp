#include "gitrepository.h"
#include "gitmanager.h"

#include <QDir>

GitRepository::GitRepository(const QString& folder, QObject *parent)
    : QObject{parent},
      mInRepository(false)
{
    mManager = new GitManager();
    setFolder(folder);
}

GitRepository::~GitRepository()
{
    delete mManager;
}

const QString &GitRepository::folder() const
{
    return mRealFolder;
}

void GitRepository::createRepository()
{
    mManager->createRepository(mRealFolder);
}

bool GitRepository::hasRepository(QString& currentBranch)
{
    currentBranch = mBranch;
    return  mInRepository;
}

bool GitRepository::add(const QString &path, QString& output)
{
    return mManager->add(mFolder,path, output);
}

bool GitRepository::remove(const QString &path, QString& output)
{
    return mManager->remove(mFolder,path, output);
}

bool GitRepository::rename(const QString &oldName, const QString &newName, QString& output)
{
    return mManager->rename(mFolder, oldName, newName,output);
}

bool GitRepository::restore(const QString &path, QString& output)
{
    return mManager->restore(mFolder, path, output);
}

QSet<QString> GitRepository::listFiles(bool refresh)
{
    if (refresh)
        update();
    return mFilesInRepositories;
}

bool GitRepository::clone(const QString &url, QString& output)
{
    return mManager->clone(mFolder,url, output);
}

bool GitRepository::commit(const QString &message, QString& output, bool autoStage)
{
    return mManager->commit(mRealFolder, message, autoStage, output);
}

bool GitRepository::revert(QString& output)
{
    return mManager->revert(mRealFolder, output);
}

void GitRepository::setFolder(const QString &newFolder)
{
    mFolder = newFolder;
    if (!newFolder.isEmpty())
        mRealFolder = mManager->rootFolder(mFolder);
    else
        mRealFolder = newFolder;
    update();
}

void GitRepository::update()
{
    if (!mManager->isValid() || mFolder.isEmpty()) {
        mInRepository = false;
        mBranch = "";
        mFilesInRepositories.clear();
        mChangedFiles.clear();
        mStagedFiles.clear();
        mConflicts.clear();
    } else {
        mInRepository = mManager->hasRepository(mRealFolder,mBranch);
        convertFilesListToSet(mManager->listFiles(mRealFolder),mFilesInRepositories);
        convertFilesListToSet(mManager->listChangedFiles(mRealFolder),mChangedFiles);
        convertFilesListToSet(mManager->listStagedFiles(mRealFolder),mStagedFiles);
        convertFilesListToSet(mManager->listConflicts(mRealFolder),mConflicts);
//        qDebug()<<"update"<<mRealFolder<<mBranch;
//        qDebug()<<mFilesInRepositories;
//        qDebug()<<mChangedFiles;
//        qDebug()<<mStagedFiles;
    }
}

const QString &GitRepository::realFolder() const
{
    return mRealFolder;
}

void GitRepository::convertFilesListToSet(const QStringList &filesList, QSet<QString> &set)
{
    set.clear();
    QDir dir(mRealFolder);
    foreach (const QString& s, filesList) {
        set.insert(cleanPath(dir.absoluteFilePath(s)));
    }
}

