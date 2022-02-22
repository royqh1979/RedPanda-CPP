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

    void add(const QString& folder, const QString& path);
    void remove(const QString& folder, const QString& path);
    void rename(const QString& folder, const QString& oldName, const QString& newName);
    void restore(const QString& folder, const QString& path);

    int logCounts(const QString& folder, const QString& branch=QString());
    QList<PGitCommitInfo> log(const QString& folder, int start, int count, const QString& branch=QString());

    QStringList listFiles(const QString& folder);
    QStringList listStagedFiles(const QString& folder);
    QStringList listChangedFiles(const QString& folder);
    QStringList listConflicts(const QString& folder);

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

    void clone(const QString& folder, const QString& url);
    void commit(const QString& folder, const QString& message, bool autoStage);
    void revert(const QString& folder);
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
