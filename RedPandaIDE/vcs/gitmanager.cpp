#include "gitmanager.h"
#include "../utils.h"

#include <QFileInfo>

GitManager::GitManager(QObject *parent) : QObject(parent),
    mGitPathValid(false)
{
}

bool GitManager::gitPathValid() const
{
    return mGitPathValid;
}

void GitManager::createRepository(const QString &folder)
{
    if (hasRepository(folder))
        throw GitError(tr("Folder \"%1\" already has a repository!"));
    QStringList args;
    args.append("init");
    runGit(folder,args);
}

bool GitManager::hasRepository(const QString &folder)
{

    QStringList args;
    args.append("status");
    QString output = runGit(folder,args);
    return !output.startsWith("fatal:");
}

void GitManager::add(const QString &folder, const QString &path)
{
    QStringList args;
    args.append("add");
    args.append(path);
    runGit(folder,args);
}

void GitManager::remove(const QString &folder, const QString &path)
{
    QStringList args;
    args.append("rm");
    args.append(path);
    runGit(folder,args);
}

void GitManager::rename(const QString &folder, const QString &oldName, const QString &newName)
{
    QStringList args;
    args.append("mv");
    args.append(oldName);
    args.append(newName);
    runGit(folder,args);
}

void GitManager::restore(const QString &folder, const QString &path)
{
    QStringList args;
    args.append("restore");
    args.append(path);
    runGit(folder,args);
}

QStringList GitManager::listFiles(const QString &folder)
{
    QStringList args;
    args.append("ls-files");
    return textToLines(runGit(folder,args));
}

void GitManager::clone(const QString &folder, const QString &url)
{
    QStringList args;
    args.append("clone");
    args.append(url);
    runGit(folder,args);
}

void GitManager::commit(const QString &folder, const QString &message)
{
    QStringList args;
    args.append("commit");
    args.append("-m");
    args.append(message);
    runGit(folder,args);
}

void GitManager::revert(const QString &folder)
{
    QStringList args;
    args.append("revert");
    runGit(folder,args);
}

void GitManager::reset(const QString &folder, const QString &commit, ResetStrategy strategy)
{
    //todo reset type
    QStringList args;
    args.append("reset");
    switch(strategy) {
    case ResetStrategy::Soft:
        args.append("--soft");
        break;
    case ResetStrategy::Hard:
        args.append("--hard");
        break;
    case ResetStrategy::Mixed:
        args.append("--mixed");
        break;
    case ResetStrategy::Merge:
        args.append("--merge");
        break;
    case ResetStrategy::Keep:
        args.append("--keep");
        break;
    }
    args.append(commit);
    runGit(folder,args);
}

void GitManager::validate()
{
    QStringList args;
    args.append("--version");
    QString output = runGit("",args);
    mGitPathValid = output.startsWith("git version");
}

QString GitManager::runGit(const QString& workingFolder, const QStringList &args)
{
    QFileInfo fileInfo(mGitPath);
    if (!fileInfo.exists())
        throw GitError("fatal: git doesn't exist");
    emit gitCmdRunning(QString("Running in \"%1\": \n \"%2\" \"%3\"")
                       .arg(workingFolder,
                            mGitPath,
                            args.join("\" \"")));
    QString output = runAndGetOutput(
                fileInfo.absoluteFilePath(),
                workingFolder,
                args);
    emit gitCmdFinished(output);
    if (output.startsWith("fatal:"))
        throw GitError(output);
    return output;
}

GitError::GitError(const QString &reason):BaseError(reason)
{

}
