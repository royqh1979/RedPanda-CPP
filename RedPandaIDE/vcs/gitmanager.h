#ifndef GITMANAGER_H
#define GITMANAGER_H

#include <QObject>
#include "utils.h"

class GitError: public BaseError {
public:
    explicit GitError(const QString& reason);
};



class GitManager : public QObject
{
    Q_OBJECT
public:
    enum class ResetStrategy {
        Soft,
        Hard,
        Merge,
        Mixed,
        Keep
    };

    explicit GitManager(QObject *parent = nullptr);

    bool gitPathValid() const;
    void createRepository(const QString& folder);
    bool hasRepository(const QString& folder);

    void add(const QString& folder, const QString& path);
    void remove(const QString& folder, const QString& path);
    void rename(const QString& folder, const QString& oldName, const QString& newName);
    void restore(const QString& folder, const QString& path);
    QStringList listFiles(const QString& folder);

    void clone(const QString& folder, const QString& url);
    void commit(const QString& folder, const QString& message);
    void revert(const QString& folder);
    void reset(const QString& folder, const QString& commit, ResetStrategy strategy);

signals:
    void gitCmdRunning(const QString& gitCmd);
    void gitCmdFinished(const QString& message);
private:
    void validate();
    QString runGit(const QString& workingFolder, const QStringList& args);
private:
    QString mGitPath;
    bool mGitPathValid;
};

#endif // GITMANAGER_H
