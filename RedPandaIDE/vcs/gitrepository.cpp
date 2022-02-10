#include "gitrepository.h"
#include "gitmanager.h"

GitRepository::GitRepository(const QString& folder, GitManager *manager, QObject *parent)
    : QObject{parent},
      mFolder(folder),
      mManager(manager)
{
    Q_ASSERT(manager!=nullptr);
}

const QString &GitRepository::folder() const
{
    return mFolder;
}

void GitRepository::createRepository()
{
    mManager->createRepository(mFolder);
}

bool GitRepository::hasRepository()
{
    return mManager->hasRepository(mFolder);
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

QStringList GitRepository::listFiles(bool refresh)
{
    if (refresh || mFiles.isEmpty()) {
        mFiles = mManager->listFiles(mFolder);
    }
    return mFiles;
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

GitManager *GitRepository::manager() const
{
    return mManager;
}

void GitRepository::setManager(GitManager *newManager)
{
    Q_ASSERT(newManager!=nullptr);
    mManager = newManager;
}
