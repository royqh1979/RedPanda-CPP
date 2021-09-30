#ifndef CODESNIPPETSMANAGER_H
#define CODESNIPPETSMANAGER_H

#include <QObject>
#include "parser/parserutils.h"
#include <QAbstractListModel>

class CodeSnippetsModel: public QAbstractListModel {
    Q_OBJECT
public:
    void addSnippet(
            const QString& caption,
            const QString& prefix,
            const QString& code,
            const QString& description,
            int menuSection);
    void remove(int index);
    void clear();

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    const QList<PCodeSnippet> &snippets() const;
    void updateSnippets(const QList<PCodeSnippet>& snippets);

private:
    QList<PCodeSnippet> mSnippets;
};

class CodeSnippetsManager : public QObject
{
    Q_OBJECT
public:
    explicit CodeSnippetsManager(QObject *parent = nullptr);

    void load();
    void save();
    const QList<PCodeSnippet> &snippets() const;

    void setSnippets(const QList<PCodeSnippet> &newSnippets);

signals:

private:
    QList<PCodeSnippet> mSnippets;
};

using PCodeSnippetManager = std::shared_ptr<CodeSnippetsManager>;

#endif // CODESNIPPETSMANAGER_H
