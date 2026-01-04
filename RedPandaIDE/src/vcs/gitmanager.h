#ifndef GITMANAGER_H
#define GITMANAGER_H

#include <QObject>
#include <QFileInfo>
#include <QSet>
#include "utils.h"
#include "gitutils.h"

class GitError: public BaseError {
public:
    explicit GitError(const QString& reason);
};

class GitManager : public QObject
{
    Q_OBJECT
public:

    explicit GitManager(QObject *parent = nullptr);

    void createRepository(const QString& folder);
    bool hasRepository(const QString& folder, QString& currentBranch);

    QString rootFolder(const QString& folder);

    bool isFileInRepository(const QFileInfo& fileInfo);
    bool isFileStaged(const QFileInfo& fileInfo);
    bool isFileChanged(const QFileInfo& fileInfo);

    bool add(const QString& folder, const QString& path, QString& output);
    bool remove(const QString& folder, const QString& path, QString& output);
    bool rename(const QString& folder, const QString& oldName, const QString& newName, QString& output);
    bool restore(const QString& folder, const QString& path, QString& output);

    int logCounts(const QString& folder, const QString& branch=QString());
    QList<PGitCommitInfo> log(const QString& folder, int start, int count, const QString& branch=QString());

    QStringList listFiles(const QString& folder);
    QStringList listStagedFiles(const QString& folder);
    QStringList listChangedFiles(const QString& folder);
    QStringList listConflicts(const QString& folder);
    QStringList listRemotes(const QString& folder);

    bool removeRemote(const QString& folder, const QString& remoteName, QString& output);
    bool renameRemote(const QString& folder, const QString& oldName,
                      const QString& newName, QString& output);
    bool addRemote(const QString& folder, const QString& name,
                   const QString& url, QString& output);
    bool setRemoteURL(const QString& folder, const QString& name,
                      const QString& newURL, QString& output);
    QString getRemoteURL(const QString& folder, const QString& name);
    QString getBranchRemote(const QString& folder, const QString& branch);
    QString getBranchMerge(const QString& folder, const QString& branch);
    bool setBranchUpstream(const QString& folder,
                           const QString& branch,
                           const QString& remoteName,
                           QString &output);

    bool fetch(const QString& folder, QString& output);
    bool pull(const QString& folder, QString& output);
    bool push(const QString& folder, QString& output);
    bool push(const QString& folder,
              const QString& remoteName,
              const QString& branch,
              QString& output);

    bool removeConfig(const QString& folder, const QString &name, QString& output);
    bool setConfig(const QString& folder, const QString &name, const QString &value, QString& output);
    bool setUserName(const QString& folder, const QString& userName, QString& output);
    bool setUserEmail(const QString& folder, const QString& userEmail, QString& output);

    QString getConfig(const QString& folder, const QString& name);
    QString getUserName(const QString& folder);
    QString getUserEmail(const QString& folder);


    QStringList listBranches(const QString& folder, int& current);
    bool switchToBranch(const QString& folder, const QString& branch, bool create,
                        bool force, bool merge, bool track, bool noTrack, bool forceCreation,
                        QString& output);
    bool merge(const QString& folder, const QString& commit, bool squash, bool fastForwardOnly,
               bool noFastForward, bool noCommit,
               QString& output,
               const QString& commitMessage=QString()
               );
    bool continueMerge(const QString& folder);
    void abortMerge(const QString& folder);

    bool isSuccess(const QString& output);
    bool clone(const QString& folder, const QString& url, QString& output);
    bool commit(const QString& folder, const QString& message, bool autoStage, QString& output);
    bool revert(const QString& folder, QString& output);
    bool reset(const QString& folder, const QString& commit, GitResetStrategy strategy, QString& output);

    bool isValid();

signals:
    void gitCmdRunning(const QString& gitCmd);
    void gitCmdFinished(const QString& message);
private:
    QString runGit(const QString& workingFolder, const QStringList& args);

    QString escapeUTF8String(const QByteArray& rawString);
private:
};

#endif // GITMANAGER_H
