#include "gitmanager.h"
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
    contents.append("*.");

    QDir dir(folder);
    stringsToFile(contents,dir.filePath(".gitignore"));
    add(folder,".gitignore");
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
    QString output = runAndGetOutput(
                fileInfo.absoluteFilePath(),
                workingFolder,
                args);
    output = escapeUTF8String(output.toUtf8());
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
