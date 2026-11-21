/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#ifndef CLASSBROWSER_H
#define CLASSBROWSER_H

#include <QAbstractItemModel>
#include "../parser/cppparser.h"

struct ClassBrowserNode : public std::enable_shared_from_this<ClassBrowserNode> {
    ClassBrowserNode* parent;
    PStatement statement;
    QVector<ClassBrowserNode *> children;
};

using PClassBrowserNode = std::shared_ptr<ClassBrowserNode>;

class ColorSchemeItem;

class ClassBrowserModel : public QAbstractItemModel{
    Q_OBJECT
        // QAbstractItemModel interface
public:
    explicit ClassBrowserModel(QObject* parent=nullptr);
    ~ClassBrowserModel();
    ClassBrowserModel& operator=(const ClassBrowserModel& model) = delete;

    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    bool hasChildren(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
//    void fetchMore(const QModelIndex &parent) override;
//    bool canFetchMore(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    const PCppParser &parser() const;
    void setParser(const PCppParser &newCppParser);
    void clear();
    const QString &currentFile() const;
    void setCurrentFile(const QString &newCurrentFile);
    void beginUpdate();
    void endUpdate();

    const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > &colors() const;
    void setColors(const std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > &newColors);

    QModelIndex modelIndexForStatement(const QString& key) const;
    QModelIndex modelIndexForStatement(const PStatement& statement) const;
    QString generateStatementKey(const PStatement& statement) const;
signals:
    void refreshStarted();
    void refreshEnd();
public slots:
    void fillStatements();
private:
    PClassBrowserNode addChild(ClassBrowserNode* node, const PStatement& statement);
    void removeNode(ClassBrowserNode *node);
    void buildStatementTree();
    void sortNode(ClassBrowserNode * node);
    void updateChildren(ClassBrowserNode *node, const StatementList& statements);
    bool shouldShowStatement(ClassBrowserNode *node, const PStatement &statement) const;
    QModelIndex indexForNode(ClassBrowserNode *node) const;
private:
    ClassBrowserNode * mRoot;
    QHash<QString,PClassBrowserNode> mNodeIndex;
    QSet<PStatement> mTopLevelStatements;
    QSet<PStatement> mNestedStatements;
    QVector<PClassBrowserNode> mNodes;
    PCppParser mParser;
    bool mUpdating;
    int mUpdateCount;
    mutable QRecursiveMutex mMutex;
    QString mCurrentFile;
    std::shared_ptr<QHash<StatementKind, std::shared_ptr<ColorSchemeItem> > > mColors;

};

#endif // CLASSBROWSER_H
