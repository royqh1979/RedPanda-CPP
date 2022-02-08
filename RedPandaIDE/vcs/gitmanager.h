#ifndef GITMANAGER_H
#define GITMANAGER_H

#include <QObject>

class GitManager : public QObject
{
    Q_OBJECT
public:
    explicit GitManager(QObject *parent = nullptr);

    bool gitPathValid() const;
    bool createRepository(const QString& folder);
    bool hasRepository(const QString& folder);


signals:
private:
    void validate();
private:
    QString mGitPath;
    bool mGitPathValid;

};

#endif // GITMANAGER_H
