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
#include "../utils.h"

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


class SearchResultModel;

class SearchResults{
public:
    explicit SearchResults(const QString &keyword, QSynedit::SearchOptions options,
                           bool useRegex, SearchFileScope scope, SearchType type,
                           const QString& folder, const QString& filters,
                           bool searchSubFolders);
    explicit SearchResults(const QString& keyword,
                           const QString& symbolFullname,
                           const QString& filename,
                           SearchType searchType,
                           SearchFileScope scope);
    const QSynedit::SearchOptions &options() const;
    void setOptions(const QSynedit::SearchOptions &newOptions);

    bool useRegex() const;
    void setUseRegex(bool newUseRegex);
    const QString &keyword() const;
    void setKeyword(const QString &newKeyword);
    SearchFileScope scope() const;
    void setScope(SearchFileScope newScope);
    SearchType searchType() const;
    void setSearchType(SearchType newSearchType);
    const QString &filename() const;
    void setFilename(const QString &newFilename);
    const QString &folder() const;
    void setFolder(const QString &newFolder);
    const QString &filters() const;
    void setFilters(const QString &newFilters);
    bool searchSubfolders() const;
    void setSearchSubfolders(bool newSearchSubfolders);
    const QList<PSearchResultTreeItem> &results() const;
    PSearchResultTreeItem result(int idx) const;
    const QString &symbolFullname() const;
    void setSymbolFullname(const QString &newSymbolFullname);

private:
    QSynedit::SearchOptions mOptions;
    bool mUseRegex;
    QString mKeyword;
    QString mSymbolFullname;
    SearchFileScope mScope;
    SearchType mSearchType;
    QString mFilename;
    QString mFolder;
    QString mFilters;
    bool mSearchSubfolders;
    QList<PSearchResultTreeItem> mResults;

    friend SearchResultModel;
};

using PSearchResults = std::shared_ptr<SearchResults>;

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

private:
    SearchResultModel *mSearchResultModel;
    bool mSelectable;
private:
    void resetModel();
    void beginAddItem();
    void endAddItem();
    // QAbstractItemModel interface
public:
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    // QAbstractItemModel interface
public:
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
friend class SearchResultModel;
};

class SearchResultModel : public QAbstractListModel {
    Q_OBJECT
public:
    explicit SearchResultModel(QObject* parent=nullptr);
    PSearchResults addSearchResults(const QString& keyword,QSynedit::SearchOptions options,
                                    bool useRegex,
                                    SearchFileScope scope, const QString& folder=QString(), const QString& filters=QString(), bool searchSubfolders=true);
    PSearchResults addSearchResults(
            const QString& keyword,
            const QString& symbolFullname,
            SearchFileScope scope);
    PSearchResults results(int index);
    int currentIndex() const;
    int resultsCount() const;
    PSearchResults currentResults();
    void addResultToSearchResults(PSearchResults results, PSearchResultTreeItem item);
    void setCurrentIndex(int index);
    void clear();
    void removeSearchResults(int index);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    SearchResultTreeModel *treeModel() const;
signals:
    void currentIndexChanged();
private:
    QList<PSearchResults> mSearchResults;
    int mCurrentIndex;
    SearchResultTreeModel *mTreeModel;
};


class SearchResultTreeViewDelegate: public QStyledItemDelegate{
Q_OBJECT
    // QAbstractItemDelegate interface
public:
    explicit SearchResultTreeViewDelegate(SearchResultTreeModel *model,
                                          QObject* parent=nullptr);
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
private:
    SearchResultTreeModel *mModel;
};

using PSearchResultTreeViewDelegate = std::shared_ptr<SearchResultTreeViewDelegate>;

#endif // SEARCHRESULTVIEW_H
