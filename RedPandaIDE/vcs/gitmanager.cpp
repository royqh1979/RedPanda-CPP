#ifdef ENABLE_VCS
#include "gitmanager.h"
#endif
#include "../utils.h"
#include "../settings.h"

#include <QDir>
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

    QStringList contents;
    contents.append(".git");
    contents.append("*.o");
    contents.append("*.exe");
    contents.append("*.layout");
#ifdef Q_OS_UNIX
    contents.append("*.");
#endif

    QDir dir(folder);
    stringsToFile(contents,dir.filePath(".gitignore"));
    QString output;
    add(folder,".gitignore",output);
}

bool GitManager::hasRepository(const QString &folder, QString& currentBranch)
{
    if (folder.isEmpty())
        return false;
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

QString GitManager::rootFolder(const QString &folder)
{
    QStringList args;
    args.append("rev-parse");
    args.append("--show-toplevel");
    return runGit(folder,args).trimmed();
}

bool GitManager::isFileInRepository(const QFileInfo& fileInfo)
{
    QStringList args;
    args.append("ls-files");
    args.append(fileInfo.fileName());
    QString output = runGit(fileInfo.absolutePath(),args);
    return output.trimmed() == fileInfo.fileName();
}

bool GitManager::isFileStaged(const QFileInfo &fileInfo)
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

bool GitManager::add(const QString &folder, const QString &path, QString& output)
{
    QStringList args;
    args.append("add");
    args.append(path);
    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::remove(const QString &folder, const QString &path, QString& output)
{
    QStringList args;
    args.append("rm");
    args.append(path);
    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::rename(const QString &folder, const QString &oldName,
                        const QString &newName, QString& output)
{
    QStringList args;
    args.append("mv");
    args.append(oldName);
    args.append(newName);
    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::restore(const QString &folder, const QString &path, QString& output)
{
    QStringList args;
    args.append("restore");
    if (path.isEmpty())
        args.append(".");
    else
        args.append(path);
    output = runGit(folder,args);
    return isSuccess(output);
}

int GitManager::logCounts(const QString &folder, const QString &branch)
{
    QStringList args;
    args.append("rev-list");
    args.append("--count");
    if (branch.isEmpty())
        args.append("HEAD");
    else
        args.append(branch);
    QString s = runGit(folder,args).trimmed();
    bool ok;
    int result = s.toInt(&ok);
    if (!ok)
        result = 0;
    return result;
}

QList<PGitCommitInfo> GitManager::log(const QString &folder, int start, int count, const QString &branch)
{
    QStringList args;
    args.append("log");
    args.append("--skip");
    args.append(QString("%1").arg(start));
    args.append("-n");
    args.append(QString("%1").arg(count));
    args.append("--format=medium");
    args.append("--date=iso-strict");
    if (branch.isEmpty())
        args.append("HEAD");
    else
        args.append(branch);
    QString output = runGit(folder,args);
    QStringList lines = textToLines(output);
    QList<PGitCommitInfo> result;
    int pos = 0;
    PGitCommitInfo commitInfo;
    while (pos<lines.length()) {
        if (lines[pos].startsWith("commit ")) {
            commitInfo = std::make_shared<GitCommitInfo>();
            commitInfo->commitHash=lines[pos].mid(QString("commit ").length()).trimmed();
            result.append(commitInfo);
        } else if(!commitInfo) {
            break;
        } else if (lines[pos].startsWith("Author:")) {
            commitInfo->author=lines[pos].mid(QString("Author:").length()).trimmed();
        } else if (lines[pos].startsWith("Date:")) {
            commitInfo->authorDate=QDateTime::fromString(lines[pos].mid(QString("Date:").length()).trimmed(),Qt::ISODate);
        } else if (!lines[pos].trimmed().isEmpty()) {
            if (commitInfo->title.isEmpty()) {
                commitInfo->title = lines[pos].trimmed();
            } else {
                commitInfo->fullCommitMessage.append(lines[pos].trimmed()+"\n");
            }
        }
        pos++;
    }
    return result;
}

QStringList GitManager::listFiles(const QString &folder)
{
    QStringList args;
    args.append("ls-files");
    return textToLines(runGit(folder,args));
}

QStringList GitManager::listStagedFiles(const QString &folder)
{
    QStringList args;
    args.append("diff");
    args.append("--staged");
    args.append("--name-only");
    return textToLines(runGit(folder,args));
}

QStringList GitManager::listChangedFiles(const QString &folder)
{
    QStringList args;
    args.append("diff");
    args.append("--name-only");
    return textToLines(runGit(folder,args));
}

QStringList GitManager::listConflicts(const QString &folder)
{
    QStringList args;
    args.append("diff");
    args.append("--name-only");
    args.append("--diff-filter=U");
    return textToLines(runGit(folder,args));
}

QStringList GitManager::listRemotes(const QString &folder)
{
    QStringList args;
    args.append("remote");
    return textToLines(runGit(folder,args));
}

bool GitManager::removeRemote(const QString &folder, const QString &remoteName, QString& output)
{
    QStringList args;
    args.append("remote");
    args.append("remove");
    args.append(remoteName);

    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::renameRemote(const QString &folder, const QString &oldName, const QString &newName, QString &output)
{
    QStringList args;
    args.append("remote");
    args.append("rename");
    args.append(oldName);
    args.append(newName);

    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::addRemote(const QString &folder, const QString &name, const QString &url, QString &output)
{
    QStringList args;
    args.append("remote");
    args.append("add");
    args.append(name);
    args.append(url);

    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::setRemoteURL(const QString &folder, const QString &name, const QString &newURL, QString &output)
{
    QStringList args;
    args.append("remote");
    args.append("set-url");
    args.append(name);
    args.append(newURL);

    output = runGit(folder,args);
    return isSuccess(output);
}

QString GitManager::getRemoteURL(const QString &folder, const QString &name)
{
    QStringList args;
    args.append("remote");
    args.append("get-url");
    args.append(name);
    return runGit(folder,args).trimmed();
}

QString GitManager::getBranchRemote(const QString &folder, const QString &branch)
{
    QStringList args;
    args.append("config");
    args.append("--get");
    args.append(QString("branch.%1.remote").arg(branch));
    return runGit(folder,args).trimmed();
}

QString GitManager::getBranchMerge(const QString &folder, const QString &branch)
{
    QStringList args;
    args.append("config");
    args.append("--get");
    args.append(QString("branch.%1.merge").arg(branch));
    return runGit(folder,args).trimmed();
}

bool GitManager::setBranchUpstream(
        const QString &folder,
        const QString &branch,
        const QString &remoteName,
        QString& output)
{
    QStringList args;
    args.append("branch");
    args.append(QString("--set-upstream-to=%1/%2").arg(remoteName,branch));
    args.append(branch);
    output = runGit(folder,args).trimmed();
    return isSuccess(output);
}

bool GitManager::fetch(const QString &folder, QString &output)
{
    QStringList args;
    args.append("fetch");
    output = runGit(folder,args).trimmed();
    return isSuccess(output);
}

bool GitManager::pull(const QString &folder, QString &output)
{
    QStringList args;
    args.append("pull");
    output = runGit(folder,args).trimmed();
    return isSuccess(output);
}

bool GitManager::push(const QString &folder, QString &output)
{
    QStringList args;
    args.append("push");
    output = runGit(folder,args).trimmed();
    return isSuccess(output);
}

bool GitManager::push(const QString &folder, const QString &remoteName, const QString &branch, QString &output)
{
    QStringList args;
    args.append("push");
    args.append("--set-upstream");
    args.append(remoteName);
    args.append(branch);
    output = runGit(folder,args).trimmed();
    return isSuccess(output);
}

bool GitManager::removeConfig(const QString &folder, const QString &name, QString &output)
{
    QStringList args;
    args.append("config");
    args.append("--unset-all");
    args.append(name);
    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::setConfig(const QString &folder, const QString &name, const QString &value, QString &output)
{
    removeConfig(folder,name,output);
    QStringList args;
    args.append("config");
    args.append("--add");
    args.append(name);
    args.append(value);
    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::setUserName(const QString &folder, const QString &userName, QString &output)
{
    return setConfig(folder,"user.name",userName,output);
}

bool GitManager::setUserEmail(const QString &folder, const QString &userEmail, QString &output)
{
    return setConfig(folder,"user.email",userEmail,output);
}

QString GitManager::getConfig(const QString& folder, const QString &name)
{
    QStringList args;
    args.append("config");
    args.append("--get");
    args.append(name);
    return runGit(folder,args).trimmed();
}

QString GitManager::getUserName(const QString& folder)
{
    return getConfig(folder, "user.name");
}

QString GitManager::getUserEmail(const QString& folder)
{
    return getConfig(folder, "user.email");
}

QStringList GitManager::listBranches(const QString &folder, int &current)
{
    QStringList args;
    args.append("branch");
    args.append("-a");
    args.append("-l");
    QStringList temp = textToLines(runGit(folder,args));
    current = -1;
    for (int i=0;i<temp.length();i++) {
        QString s = temp[i];
        if (s.startsWith('*')) {
            current = i;
            temp[i] = s.mid(1).trimmed();
        } else if (s.startsWith('+')) {
            temp[i] = s.mid(1).trimmed();
        } else {
            temp[i] = s.trimmed();
        }
    }
    return temp;
}

bool GitManager::switchToBranch(const QString &folder, const QString &branch,
                                bool create, bool force, bool merge, bool track,
                                bool noTrack, bool forceCreation, QString& output)
{
    QStringList args;
    args.append("switch");
    if (forceCreation)
        args.append("-C");
    else if (create)
        args.append("-c");
    if (merge)
        args.append("-m");
    if (force)
        args.append("-f");
    if (track)
        args.append("--track");
    else if (noTrack)
        args.append("--no-track");
    args.append(branch);
    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::merge(const QString &folder, const QString &commit, bool squash,
                       bool fastForwardOnly, bool noFastForward, bool noCommit,
                       QString& output,
                       const QString& commitMessage)
{
    QStringList args;
    args.append("merge");
    if (squash)
        args.append("--squash");
    if (fastForwardOnly)
        args.append("--ff-only");
    else if (noFastForward)
        args.append("--no-ff");
    if (noCommit)
        args.append("--no-commit");
    if (!commitMessage.isEmpty()
            && commitMessage != QObject::tr("<Auto Generated by Git>")){
        args.append("-m");
        args.append(commitMessage);
    }
    args.append(commit);
    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::continueMerge(const QString &folder)
{
    QStringList args;
    args.append("merge");
    args.append("--continue");
    QString output = runGit(folder,args);
    return isSuccess(output);

}

void GitManager::abortMerge(const QString &folder)
{
    QStringList args;
    args.append("merge");
    args.append("--abort");
    runGit(folder,args);
}

bool GitManager::isSuccess(const QString &output)
{
    QStringList lst = textToLines(output);
    if (!lst.isEmpty()) {
        foreach (const QString& s, lst) {
            QString last= s.trimmed();
            if (last.startsWith("error:") || last.startsWith("fatal:"))
                return false;
        }
        return true;
    }
    return true;
}

bool GitManager::clone(const QString &folder, const QString &url, QString& output)
{
    QStringList args;
    args.append("clone");
    args.append(url);
    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::commit(const QString &folder, const QString &message, bool autoStage, QString& output)
{
    QStringList args;
    args.append("commit");
    if (autoStage)
        args.append("-a");
    args.append("-m");
    args.append(message);
    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::revert(const QString &folder, QString& output)
{
    QStringList args;
    args.append("revert");
    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::reset(const QString &folder, const QString &commit,
                       GitResetStrategy strategy,
                       QString& output)
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
    output = runGit(folder,args);
    return isSuccess(output);
}

bool GitManager::isValid()
{
    return pSettings->vcs().gitOk();
}

QString GitManager::runGit(const QString& workingFolder, const QStringList &args)
{
    if (!isValid())
        return "";
    QFileInfo fileInfo(pSettings->vcs().gitPath());
    if (!fileInfo.exists())
        return "fatal: git doesn't exist";
    emit gitCmdRunning(QString("Running in \"%1\": \n \"%2\" \"%3\"")
                       .arg(workingFolder,
                            pSettings->vcs().gitPath(),
                            args.join("\" \"")));
//    qDebug()<<"---------";
//    qDebug()<<args;
    QProcessEnvironment env;
#ifdef Q_OS_WIN
    env.insert("PATH",pSettings->dirs().appDir());
    env.insert("GIT_ASKPASS",includeTrailingPathDelimiter(pSettings->dirs().appDir())+"redpanda-win-git-askpass.exe");
#else // Unix
    env.insert(QProcessEnvironment::systemEnvironment());
    env.insert("LANG","en");
    env.insert("LANGUAGE","en");
    env.insert("GIT_ASKPASS",includeTrailingPathDelimiter(pSettings->dirs().appLibexecDir())+"redpanda-git-askpass");
#endif
    QString output = runAndGetOutput(
                fileInfo.absoluteFilePath(),
                workingFolder,
                args,
                "",
                false,
                env);
    output = escapeUTF8String(output.toUtf8());
//    qDebug()<<output;
    emit gitCmdFinished(output);
//    if (output.startsWith("fatal:"))
//        throw GitError(output);
    return output;
}

QString GitManager::escapeUTF8String(const QByteArray &rawString)
{
    QByteArray stringValue;
    int p = 0;
    while (p<rawString.length()) {
        char ch = rawString[p];
        if (ch =='\\' && p+1 < rawString.length()) {
            p++;
            ch = rawString[p];
            switch (ch) {
            case '\'':
                stringValue+=0x27;
                p++;
                break;
            case '"':
                stringValue+=0x22;
                p++;
                break;
            case '?':
                stringValue+=0x3f;
                p++;
                break;
            case '\\':
                stringValue+=0x5c;
                p++;
                break;
            case 'a':
                stringValue+=0x07;
                p++;
                break;
            case 'b':
                stringValue+=0x08;
                p++;
                break;
            case 'f':
                stringValue+=0x0c;
                p++;
                break;
            case 'n':
                stringValue+=0x0a;
                p++;
                break;
            case 'r':
                stringValue+=0x0d;
                p++;
                break;
            case 't':
                stringValue+=0x09;
                p++;
                break;
            case 'v':
                stringValue+=0x0b;
                p++;
                break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            {
                int i=0;
                for (i=0;i<3;i++) {
                    if (p+i>=rawString.length() ||
                             rawString[p+i]<'0' || rawString[p+i]>'7')
                        break;
                }
                bool ok;
                unsigned char ch = rawString.mid(p,i).toInt(&ok,8);
                stringValue+=ch;
                p+=i;
                break;
            }
            }
        } else {
            if (ch!='\"')
                stringValue+=ch;
            p++;
        }
    }
    return QString::fromUtf8(stringValue);
}

GitError::GitError(const QString &reason):BaseError(reason)
{

}
