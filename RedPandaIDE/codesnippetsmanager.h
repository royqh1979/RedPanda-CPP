#ifndef CODESNIPPETSMANAGER_H
#define CODESNIPPETSMANAGER_H

#include <QObject>
#include "parser/parserutils.h"

class CodeSnippetsManager : public QObject
{
    Q_OBJECT
public:
    explicit CodeSnippetsManager(QObject *parent = nullptr);

    void load();
    void save();
    void addSnippet(
            const QString& caption,
            const QString& prefix,
            const QString& code,
            const QString& description,
            int menuSection);
    void remove(int index);
    void clear();
    const QList<PCodeSnippet> &snippets() const;

signals:
private:
    QList<PCodeSnippet> mSnippets;

private:
};

#endif // CODESNIPPETSMANAGER_H
