#include "gitrepository.h"
#include "gitmanager.h"

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

void GitRepository::add(const QString &path)
{
    mManager->add(mFolder,path);
}

void GitRepository::remove(const QString &path)
{
    mManager->remove(mFolder,path);
}

void GitRepository::rename(const QString &oldName, const QString &newName)
{
    mManager->rename(mFolder, oldName, newName);
}

void GitRepository::restore(const QString &path)
{
    mManager->restore(mFolder, path);
}

QSet<QString> GitRepository::listFiles(bool refresh)
{
    if (refresh)
        update();
    return mFilesInRepositories;
}

void GitRepository::clone(const QString &url)
{
    mManager->clone(mFolder,url);
}

void GitRepository::commit(const QString &message, bool autoAdd)
{
    if (autoAdd) {
        convertFilesListToSet(mManager->listChangedFiles(mRealFolder),mChangedFiles);
        foreach(const QString& s, mChangedFiles) {
            QFileInfo info(s);
            mManager->add(info.absolutePath(),info.fileName());
        }
    }
    mManager->commit(mRealFolder, message);
}

void GitRepository::revert()
{
    mManager->revert(mRealFolder);
}

void GitRepository::reset(const QString &commit, GitResetStrategy strategy)
{
    mManager->reset(mRealFolder,commit,strategy);
}

void GitRepository::setFolder(const QString &newFolder)
{
    mFolder = newFolder;
    mRealFolder = mManager->rootFolder(mFolder);
    update();
}

void GitRepository::update()
{
    if (!mManager->isValid()) {
        mInRepository = false;
        mBranch = "";
        mFilesInRepositories.clear();
        mChangedFiles.clear();
        mStagedFiles.clear();
    } else {
        mInRepository = mManager->hasRepository(mRealFolder,mBranch);
        convertFilesListToSet(mManager->listFiles(mRealFolder),mFilesInRepositories);
        convertFilesListToSet(mManager->listChangedFiles(mRealFolder),mChangedFiles);
        convertFilesListToSet(mManager->listStagedFiles(mRealFolder),mStagedFiles);
    }
}

const QString &GitRepository::realFolder() const
{
    return mRealFolder;
}

void GitRepository::convertFilesListToSet(const QStringList &filesList, QSet<QString> &set)
{
    set.clear();
    foreach (const QString& s, filesList) {
        set.insert(includeTrailingPathDelimiter(mRealFolder)+s);
    }
}

