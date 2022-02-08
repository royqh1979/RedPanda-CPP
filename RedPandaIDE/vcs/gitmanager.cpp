#include "gitmanager.h"

GitManager::GitManager(QObject *parent) : QObject(parent),
    mGitPathValid(false)
{
}

bool GitManager::gitPathValid() const
{
    return mGitPathValid;
}
