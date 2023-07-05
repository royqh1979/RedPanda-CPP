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
#ifndef SEARCHRESULTVIEW_H
#define SEARCHRESULTVIEW_H

#include <QTreeView>
#include <QMap>
#include <QStyledItemDelegate>
#include "qsynedit/searcher/baseseacher.h"
#include "utils.h"

#define MAX_SEARCH_RESULTS 20
struct SearchResultTreeItem;
using PSearchResultTreeItem = std::shared_ptr<SearchResultTreeItem>;
using SearchResultTreeItemList = QList<PSearchResultTreeItem>;
using PSearchResultTreeItemList = std::shared_ptr<SearchResultTreeItemList>;

enum class SearchType {
    Search,
    FindOccurences
};

struct SearchResultTreeItem {
    QString filename;
    int line;
    int start;
    int len;
    QString text;
    SearchResultTreeItem* parent;
    SearchResultTreeItemList results;
    bool selected;
};

struct SearchResults{
    QSynedit::SearchOptions options;
    QString keyword;
    QString statementFullname;
    SearchFileScope scope;
    SearchType searchType;
    QString filename;
    QString folder;
    QString filters;
    bool searchSubfolders;
    QList<PSearchResultTreeItem> results;
};

using PSearchResults = std::shared_ptr<SearchResults>;

class SearchResultModel : public QObject {
    Q_OBJECT
public:
    explicit SearchResultModel(QObject* parent=nullptr);
    PSearchResults addSearchResults(const QString& keyword,QSynedit::SearchOptions options,
                                    SearchFileScope scope, const QString& folder=QString(), const QString& filters=QString(), bool searchSubfolders=true);
    PSearchResults addSearchResults(
            const QString& keyword,
            const QString& symbolFullname,
            SearchFileScope scope);
    PSearchResults results(int index);
    void notifySearchResultsUpdated();
    int currentIndex() const;
    int resultsCount() const;
    PSearchResults currentResults();
    void setCurrentIndex(int index);
    void clear();
    void removeSearchResults(int index);
signals:
    void modelChanged();
    void currentChanged(int index);
private:
    QList<PSearchResults> mSearchResults;
    int mCurrentIndex;

};

class SearchResultListModel: public QAbstractListModel {
Q_OBJECT
    // QAbstractItemModel interface
public:
    explicit SearchResultListModel(SearchResultModel* model,QObject* parent=nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
public slots:
    void onResultModelChanged();
private:
    SearchResultModel *mSearchResultModel;
};

using PSearchResultListModel = std::shared_ptr<SearchResultListModel>;

class SearchResultTreeModel : public QAbstractItemModel {
Q_OBJECT
    // QAbstractItemModel interface
public:
    explicit SearchResultTreeModel(SearchResultModel* model,QObject* parent=nullptr);
    QModelIndex index(int row, int column, const QModelIndex &parent) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    SearchResultModel *searchResultModel() const;
    bool getItemFileAndLineChar(
            const QModelIndex&index,
            QString& filename,
            int& line,
            int& startChar);
    bool selectable() const;
    void setSelectable(bool newSelectable);

public slots:
    void onResultModelChanged();
private:
    SearchResultModel *mSearchResultModel;
    bool mSelectable;

    // QAbstractItemModel interface
public:
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // QAbstractItemModel interface
public:
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
};

using PSearchResultTreeModel = std::shared_ptr<SearchResultTreeModel>;

class SearchResultTreeViewDelegate: public QStyledItemDelegate{
Q_OBJECT
    // QAbstractItemDelegate interface
public:
    explicit SearchResultTreeViewDelegate(PSearchResultTreeModel model,
                                          QObject* parent=nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
private:
    PSearchResultTreeModel mModel;
};

using PSearchResultTreeViewDelegate = std::shared_ptr<SearchResultTreeViewDelegate>;

#endif // SEARCHRESULTVIEW_H
