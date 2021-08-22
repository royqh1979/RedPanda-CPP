#ifndef CLASSBROWSER_H
#define CLASSBROWSER_H

#include <QAbstractItemModel>
#include "parser/cppparser.h"

struct ClassBrowserNode {
    ClassBrowserNode* parent;
    PStatement statement;
    QVector<ClassBrowserNode *> children;
    bool childrenFetched;
};

using PClassBrowserNode = std::shared_ptr<ClassBrowserNode>;

class ClassBrowserModel : public QAbstractItemModel{
    Q_OBJECT
        // QAbstractItemModel interface
public:
    explicit ClassBrowserModel(QObject* parent=nullptr);
    ~ClassBrowserModel();
    ClassBrowserModel& operator=(const ClassBrowserModel& model) = delete;

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    void fetchMore(const QModelIndex &parent) override;
    bool canFetchMore(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    const PCppParser &cppParser() const;
    void setCppParser(const PCppParser &newCppParser);
    void clear();
public slots:
    void fillStatements();
private:
    void addChild(ClassBrowserNode* node, PStatement statement);
    void addMembers(const QSet<QString>& includedFiles);
    void filterChildren(ClassBrowserNode * node, const StatementMap& statements);
private:
    ClassBrowserNode * mRoot;
    QHash<QString,PStatement> mDummyStatements;
    QVector<PClassBrowserNode> mNodes;
    PCppParser mParser;
    bool mUpdating;
    int mUpdateCount;
    QRecursiveMutex mMutex;
    QString mCurrentFile;
    bool mShowInheritedMembers;

};

#endif // CLASSBROWSER_H
