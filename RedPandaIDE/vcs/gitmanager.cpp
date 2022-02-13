#include "gitmanager.h"
#include "../utils.h"
#include "../settings.h"

#include <QFileInfo>

GitManager::GitManager(QObject *parent) : QObject(parent)
{
}

void GitManager::createRepository(const QString &folder)
{
    QString currentBranch;
    if (hasRepository(folder,currentBranch))
        throw GitError(tr("Folder \"%1\" already has a repository!"));
    QStringList args;
    args.append("init");
    runGit(folder,args);
}

bool GitManager::hasRepository(const QString &folder, QString& currentBranch)
{

    QStringList args;
    args.append("status");
    args.append("-b");
    args.append("-u");
    args.append("no");
    args.append("--ignored=no");
    QString output = runGit(folder,args);
    bool result = output.startsWith("On branch");
    if (result) {
        int pos = QString("On branch").length();
        while (pos<output.length() && output[pos].isSpace())
            pos++;
        int endPos = pos;
        while (endPos<output.length() && !output[endPos].isSpace())
            endPos++;
        currentBranch = output.mid(pos,endPos-pos);
    }
    return result;
}

bool GitManager::isFileInRepository(const QFileInfo& fileInfo)
{
    QStringList args;
    args.append("ls-files");
    args.append(fileInfo.fileName());
    QString output = runGit(fileInfo.absolutePath(),args);
    return output.trimmed() == fileInfo.fileName();
}

bool GitManager::isFileInStaged(const QFileInfo &fileInfo)
{
    QStringList args;
    args.append("diff");
    args.append("--staged");
    args.append("--name-only");
    args.append(fileInfo.fileName());
    QString output = runGit(fileInfo.absolutePath(),args);
    return output.trimmed() == fileInfo.fileName();
}

bool GitManager::isFileChanged(const QFileInfo &fileInfo)
{
    QStringList args;
    args.append("diff");
    args.append("--name-only");
    args.append(fileInfo.fileName());
    QString output = runGit(fileInfo.absolutePath(),args);
    return output.trimmed() == fileInfo.fileName();
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

void GitManager::reset(const QString &folder, const QString &commit, GitResetStrategy strategy)
{
    //todo reset type
    QStringList args;
    args.append("reset");
    switch(strategy) {
    case GitResetStrategy::Soft:
        args.append("--soft");
        break;
    case GitResetStrategy::Hard:
        args.append("--hard");
        break;
    case GitResetStrategy::Mixed:
        args.append("--mixed");
        break;
    case GitResetStrategy::Merge:
        args.append("--merge");
        break;
    case GitResetStrategy::Keep:
        args.append("--keep");
        break;
    }
    args.append(commit);
    runGit(folder,args);
}

QString GitManager::runGit(const QString& workingFolder, const QStringList &args)
{
    QFileInfo fileInfo(pSettings->vcs().gitPath());
    if (!fileInfo.exists())
        return "fatal: git doesn't exist";
    emit gitCmdRunning(QString("Running in \"%1\": \n \"%2\" \"%3\"")
                       .arg(workingFolder,
                            pSettings->vcs().gitPath(),
                            args.join("\" \"")));
    QString output = runAndGetOutput(
                fileInfo.absoluteFilePath(),
                workingFolder,
                args);
    emit gitCmdFinished(output);
//    if (output.startsWith("fatal:"))
//        throw GitError(output);
    return output;
}

GitError::GitError(const QString &reason):BaseError(reason)
{

}
