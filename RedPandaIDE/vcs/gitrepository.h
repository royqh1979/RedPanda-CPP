#ifndef GITREPOSITORY_H
#define GITREPOSITORY_H

#include <QObject>
#include <memory>

enum class GitResetStrategy {
    Soft,
    Hard,
    Merge,
    Mixed,
    Keep
};

class GitManager;
class GitRepository : public QObject
{
    Q_OBJECT
public:
    explicit GitRepository(const QString& folder, GitManager* manager, QObject *parent = nullptr);

    const QString &folder() const;

    void createRepository();
    bool hasRepository(QString& currentBranch);

    void add(const QString& path);
    void remove(const QString& path);
    void rename(const QString& oldName, const QString& newName);
    void restore(const QString& path);
    QStringList listFiles(bool refresh);

    void clone(const QString& url);
    void commit(const QString& message);
    void revert();
    void reset(const QString& commit, GitResetStrategy strategy);

    GitManager *manager() const;
    void setManager(GitManager *newManager);

    void setFolder(const QString &newFolder);

signals:
private:
    QString mFolder;
    GitManager* mManager;
    QStringList mFiles;
};

#endif // GITREPOSITORY_H
