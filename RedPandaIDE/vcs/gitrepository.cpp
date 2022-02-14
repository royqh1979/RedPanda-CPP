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
    return mFolder;
}

void GitRepository::createRepository()
{
    mManager->createRepository(mFolder);
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

void GitRepository::commit(const QString &message)
{
    mManager->commit(mFolder, message);
}

void GitRepository::revert()
{
    mManager->revert(mFolder);
}

void GitRepository::reset(const QString &commit, GitResetStrategy strategy)
{
    mManager->reset(mFolder,commit,strategy);
}

void GitRepository::setFolder(const QString &newFolder)
{
    mFolder = newFolder;
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
        mInRepository = mManager->hasRepository(mFolder,mBranch);
        convertFilesListToSet(mManager->listFiles(mFolder),mFilesInRepositories);
        convertFilesListToSet(mManager->listChangedFiles(mFolder),mChangedFiles);
        convertFilesListToSet(mManager->listStagedFiles(mFolder),mStagedFiles);
    }
}

void GitRepository::convertFilesListToSet(const QStringList &filesList, QSet<QString> &set)
{
    set.clear();
    foreach (const QString& s, filesList) {
        set.insert(includeTrailingPathDelimiter(mFolder)+s);
    }
}

