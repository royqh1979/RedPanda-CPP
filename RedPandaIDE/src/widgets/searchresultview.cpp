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
#include "searchresultview.h"
#include <QApplication>
#include <QPainter>
#include <QStyledItemDelegate>
#include "../mainwindow.h"

PSearchResults SearchResultModel::addSearchResults(
        const QString &keyword, QSynedit::SearchOptions options,
        bool useRegex,
        SearchFileScope scope, const QString& folder, const QString& filters, bool searchSubfolders)
{
    mTreeModel->beginResetModel();
    int index=-1;
    for (int i=0;i<mSearchResults.size();i++) {
        PSearchResults results = mSearchResults[i];
        if (results->keyword() == keyword && results->scope() == scope
                && results->searchType() == SearchType::Search) {
            index=i;
            break;
        }
    }
    if (index>=0) {
        beginRemoveRows(QModelIndex(),index,index);
        mSearchResults.removeAt(index);
        endRemoveRows();
    }
    if (mSearchResults.size()>=MAX_SEARCH_RESULTS) {
        int idx = mSearchResults.size()-1;
        beginRemoveRows(QModelIndex(), idx,idx);
        mSearchResults.pop_back();
        endRemoveRows();
    }
    PSearchResults results = std::make_shared<SearchResults>(
                keyword,options,useRegex,scope,
                SearchType::Search,folder,filters,searchSubfolders);
    beginInsertRows(QModelIndex(),0,0);
    mSearchResults.push_front(results);
    endInsertRows();
    mCurrentIndex = 0;
    mTreeModel->endResetModel();
    emit currentIndexChanged();
    return results;
}

PSearchResults SearchResultModel::addSearchResults(
        const QString& keyword,
        const QString& symbolFullname,
        SearchFileScope scope)
{
    mTreeModel->beginResetModel();
    int index=-1;
    for (int i=0;i<mSearchResults.size();i++) {
        PSearchResults results = mSearchResults[i];
        if (results->searchType() == SearchType::FindOccurences
                && results->scope() == scope
                && results->symbolFullname() == symbolFullname
                ) {
            index=i;
            break;
        }
    }
    if (index>=0) {
        beginRemoveRows(QModelIndex(),index,index);
        mSearchResults.removeAt(index);
        endRemoveRows();
    }
    if (mSearchResults.size()>=MAX_SEARCH_RESULTS) {
        int idx = mSearchResults.size()-1;
        beginRemoveRows(QModelIndex(), idx,idx);
        mSearchResults.pop_back();
        endRemoveRows();
    }
    PSearchResults results = std::make_shared<SearchResults>(
                keyword, symbolFullname,
                "", SearchType::FindOccurences,scope);
    beginInsertRows(QModelIndex(),0,0);
    mSearchResults.push_front(results);
    endInsertRows();
    mCurrentIndex = 0;
    mTreeModel->endResetModel();
    emit currentIndexChanged();
    return results;
}

PSearchResults SearchResultModel::results(int index)
{
    if (index<0 || index>=mSearchResults.size()) {
        return PSearchResults();
    }
    return mSearchResults[index];
}

SearchResultModel::SearchResultModel(QObject* parent):
    QAbstractListModel(parent),
    mCurrentIndex(-1)
{
    mTreeModel = new SearchResultTreeModel(this,this);
}

int SearchResultModel::currentIndex() const
{
    return mCurrentIndex;
}

int SearchResultModel::resultsCount() const
{
    return mSearchResults.count();
}

PSearchResults SearchResultModel::currentResults()
{
    return results(mCurrentIndex);
}

void SearchResultModel::addResultToSearchResults(PSearchResults results, PSearchResultTreeItem item)
{
    bool notifyModel = results == currentResults();
    if (notifyModel) {
        int idx = results->results().count();
        mTreeModel->beginInsertRows(QModelIndex(),idx,idx);
    }
    results->mResults.append(item);
    if (notifyModel)
        mTreeModel->endInsertRows();
}

void SearchResultModel::setCurrentIndex(int index)
{
    if (index!=mCurrentIndex &&
            index>=0 && index<mSearchResults.size()) {
        mTreeModel->beginResetModel();
        mCurrentIndex = index;
        mTreeModel->endResetModel();
        emit currentIndexChanged();
    }
}

void SearchResultModel::clear()
{
    mCurrentIndex = -1;
    mTreeModel->beginResetModel();
    beginResetModel();
    mSearchResults.clear();
    endResetModel();
    mTreeModel->endResetModel();
}

void SearchResultModel::removeSearchResults(int index)
{
    bool shouldResetTree = (index == currentIndex());
    if (shouldResetTree)
        mTreeModel->beginResetModel();
    beginRemoveRows(QModelIndex(),index,index);
    mSearchResults.removeAt(index);
    endRemoveRows();
    if (shouldResetTree)
        mTreeModel->endResetModel();
}

SearchResultTreeModel::SearchResultTreeModel(SearchResultModel *model, QObject *parent):
    QAbstractItemModel(parent),
    mSearchResultModel(model),
    mSelectable(false)
{
}

QModelIndex SearchResultTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row,column,parent))
        return QModelIndex();

    PSearchResults results = mSearchResultModel->currentResults();
    if (!results)
        return QModelIndex();
    SearchResultTreeItem *parentItem=nullptr;
    PSearchResultTreeItem childItem;
    if (!parent.isValid()) {
        parentItem = nullptr;
        childItem = results->result(row);
    } else {
        parentItem = static_cast<SearchResultTreeItem *>(parent.internalPointer());
        childItem = parentItem->results[row];
    }
    if (childItem)
        return createIndex(row,column,childItem.get());
    return QModelIndex();
}

QModelIndex SearchResultTreeModel::parent(const QModelIndex &child) const
{
    if (!child.isValid())
        return QModelIndex();
    SearchResultTreeItem* item = static_cast<SearchResultTreeItem *>(child.internalPointer());
    if (!item) {
        return QModelIndex();
    } else {
        if (item->parent==nullptr)
            return QModelIndex();
        SearchResultTreeItem* parent = item->parent;
        int row = -1;
        for (int i=0;i<parent->results.count();i++) {
            if (parent->results[i].get()==item) {
                row = i;
                break;
            }
        }
        return createIndex(row,0,parent);
    }
}

int SearchResultTreeModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()){ //root
        PSearchResults searchResults = mSearchResultModel->currentResults();
        if (!searchResults)
            return 0;
        return searchResults->results().count();
    }
    SearchResultTreeItem* item = static_cast<SearchResultTreeItem *>(parent.internalPointer());    if (!item)
        return 0;
    return item->results.count();
}

int SearchResultTreeModel::columnCount(const QModelIndex &) const
{
    return 1;
}

QVariant SearchResultTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()){
        return QVariant();
    }
    SearchResultTreeItem *item = static_cast<SearchResultTreeItem *>(index.internalPointer());
    if (!item)
        return QVariant();
    if (role == Qt::DisplayRole) {

        PSearchResults results = mSearchResultModel->currentResults();

         if (!results || !index.isValid() ) {
             // This is nothing this function is supposed to handle
             return QVariant();
         }

         if (item->parent==nullptr) { //is filename
             return QString("%1(%2)").arg(item->filename)
                     .arg(item->results.count());
         } else {
             return QString("%1 %2: %3").arg(tr("Line")).arg(item->line+1)
                 .arg(item->text);
         }
    }
    if (role == Qt::CheckStateRole && mSelectable) {

        PSearchResults results = mSearchResultModel->currentResults();

         if (!results || !index.isValid() ) {
             // This is nothing this function is supposed to handle
             return QVariant();
         }

         if (item->parent==nullptr) { //is filename
             return QVariant();
         } else {
             return (item->selected)?Qt::Checked:Qt::Unchecked;
         }
    }
    return QVariant();

}

SearchResultModel *SearchResultTreeModel::searchResultModel() const
{
    return mSearchResultModel;
}

bool SearchResultTreeModel::getItemFileAndLineChar(const QModelIndex &index, QString &filename, int &line, int &startChar)
{
    if (!index.isValid()){
        return false;
    }
    SearchResultTreeItem *item = static_cast<SearchResultTreeItem *>(index.internalPointer());
    if (!item)
        return false;

    PSearchResults results = mSearchResultModel->currentResults();

    if (!results ) {
        // This is nothing this function is supposed to handle
        return false;
    }

    SearchResultTreeItem *parent = item->parent;
    if (parent==nullptr) { //is filename
        return false;
    } else {
        filename = parent->filename;
        line = item->line;
        startChar = item->start;
        return true;
    }
    return false;
}

Qt::ItemFlags SearchResultTreeModel::flags(const QModelIndex &) const
{
    Qt::ItemFlags flags=Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (mSelectable) {
        flags.setFlag(Qt::ItemIsUserCheckable);
    }
    return flags;
}

bool SearchResultTreeModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()){
        return false;
    }
    SearchResultTreeItem *item = static_cast<SearchResultTreeItem *>(index.internalPointer());
    if (!item)
        return false;
    if (role == Qt::CheckStateRole && mSelectable) {
        PSearchResults results = mSearchResultModel->currentResults();

         if (!results || !index.isValid() ) {
             // This is nothing this function is supposed to handle
             return false;
         }

         if (item->parent==nullptr) { //is filename
             return false;
         } else {
             item->selected = value.toBool();
             return true;
         }
    }
    return false;

}

bool SearchResultTreeModel::selectable() const
{
    return mSelectable;
}

void SearchResultTreeModel::setSelectable(bool newSelectable)
{
    if (newSelectable!=mSelectable) {
        mSelectable = newSelectable;
    }
    beginResetModel();
    if (mSelectable) {
        //select all items by default
        PSearchResults results = mSearchResultModel->currentResults();
        if (results) {
            foreach (const PSearchResultTreeItem& file, results->results()) {
                file->selected = false;
                foreach (const PSearchResultTreeItem& item, file->results) {
                    item->selected = true;
                }
            }
        }
    }
    endResetModel();
}

int SearchResultModel::rowCount(const QModelIndex &) const
{
    return resultsCount();
}

QVariant SearchResultModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole) {
        PSearchResults searchResults = mSearchResults[index.row()];
        if (!searchResults)
            return QVariant();
        if (searchResults->searchType() == SearchType::Search) {
            switch (searchResults->scope()) {
            case SearchFileScope::currentFile:
                return tr("Current File:") + QString(" \"%1\"").arg(searchResults->keyword());
            case SearchFileScope::wholeProject:
                return tr("Files In Project:") + QString(" \"%1\"").arg(searchResults->keyword());
            case SearchFileScope::openedFiles:
                return tr("Open Files:") + QString(" \"%1\"").arg(searchResults->keyword());
            case SearchFileScope::Folder:
                return tr("\"%1\" in Folder \"%2\"").arg(searchResults->keyword(),searchResults->folder());
            }
        } else if (searchResults->searchType() == SearchType::FindOccurences) {
            if (searchResults->scope() == SearchFileScope::currentFile) {
                return tr("Find Usages in Current File: '%1'")
                    .arg(searchResults->keyword());
            } else {
                return tr("Find Usages in Project: '%1'")
                    .arg(searchResults->keyword());
            }
        }
    }
    return QVariant();
}

SearchResultTreeModel *SearchResultModel::treeModel() const
{
    return mTreeModel;
}

/**
 *
 * see https://stackoverflow.com/questions/1956542/how-to-make-item-view-render-rich-html-text-in-qt/66412883#66412883
 */
SearchResultTreeViewDelegate::SearchResultTreeViewDelegate(SearchResultTreeModel *model, QObject *parent):
    QStyledItemDelegate(parent),
    mModel(model)
{

}

void SearchResultTreeViewDelegate::paint(QPainter *painter, const QStyleOptionViewItem &optIn, const QModelIndex &index) const
{
    QStyleOptionViewItem option = optIn;
    initStyleOption(&option,index);
    PSearchResults results = mModel->searchResultModel()->currentResults();

     if (!results || !index.isValid() ) {
         // This is nothing this function is supposed to handle
         return;
     }

     QStyle *style = option.widget ? option.widget->style() : QApplication::style();

     // Painting item without text (this takes care of painting e.g. the highlighted for selected
     // or hovered over items in an ItemView)
     option.text = QString();
     style->drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);
     SearchResultTreeItem* item = static_cast<SearchResultTreeItem *>(index.internalPointer());

     QString fullText;
     if (item->parent==nullptr) { //is filename
         fullText = QString("%1(%2)").arg(item->filename)
                     .arg(item->results.count());
     } else {
         fullText = QString("%1 %2: %3").arg(tr("Line")).arg(item->line)
             .arg(item->text);
     }
     // Figure out where to render the text in order to follow the requested alignment
     option.text = fullText;
     QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &option);

     QFontMetrics metrics = option.fontMetrics;
     int x=textRect.left();
     int y=textRect.top() + metrics.ascent();
     if (item->parent==nullptr) { //is filename
        painter->drawText(x,y,fullText);
     } else {
         QFont oldFont = painter->font();
         QPen oldPen = painter->pen();
         QFont font1 = option.font;
         font1.setBold(false);
         painter->setFont(font1);
         QString s = item->text.mid(0,item->start);
         QString text = QString("%1 %2: %3").arg(tr("Line")).arg(item->line+1)
                 .arg(s);
         painter->drawText(x,y,text);
         metrics = QFontMetrics(font1);
         x+=metrics.horizontalAdvance(text);
         QFont font2 = option.font;
         font2.setBold(true);
         painter->setFont(font2);
         text=item->text.mid(item->start,item->len);
         metrics = QFontMetrics(font2);
         int width = metrics.horizontalAdvance(text);
         painter->setPen(qApp->palette().color(QPalette::ColorRole::ButtonText));
         QRect rect = textRect;
         rect.setLeft(x);
         rect.setWidth(width);
         painter->fillRect(rect,qApp->palette().color(QPalette::ColorRole::Button));
         painter->drawText(x,y,text);
         metrics = QFontMetrics(font2);
         x+=width;
         painter->setPen(oldPen);
         text = item->text.mid(item->start+item->len);
         painter->setFont(font1);
         painter->drawText(x,y,text);
         painter->setFont(oldFont);

     }


}

SearchResults::SearchResults(const QString &keyword, QSynedit::SearchOptions options, bool useRegex, SearchFileScope scope, SearchType type, const QString &folder, const QString &filters, bool searchSubFolders)
{
    mKeyword = keyword;
    mOptions = options;
    mUseRegex = useRegex;
    mScope = scope;
    mSearchType = SearchType::Search;
    mFolder=folder;
    mFilters=filters;
    mSearchSubfolders=searchSubFolders;
}

SearchResults::SearchResults(const QString &keyword, const QString &symbolFullname, const QString &filename, SearchType searchType, SearchFileScope scope)
{
    mKeyword = keyword;
    mSymbolFullname = symbolFullname;
    mFilename = "";
    mSearchType = SearchType::FindOccurences;
    mScope = scope;
}

const QSynedit::SearchOptions &SearchResults::options() const
{
    return mOptions;
}

void SearchResults::setOptions(const QSynedit::SearchOptions &newOptions)
{
    mOptions = newOptions;
}

bool SearchResults::useRegex() const
{
    return mUseRegex;
}

void SearchResults::setUseRegex(bool newUseRegex)
{
    mUseRegex = newUseRegex;
}

const QString &SearchResults::keyword() const
{
    return mKeyword;
}

void SearchResults::setKeyword(const QString &newKeyword)
{
    mKeyword = newKeyword;
}

SearchFileScope SearchResults::scope() const
{
    return mScope;
}

void SearchResults::setScope(SearchFileScope newScope)
{
    mScope = newScope;
}

SearchType SearchResults::searchType() const
{
    return mSearchType;
}

void SearchResults::setSearchType(SearchType newSearchType)
{
    mSearchType = newSearchType;
}

const QString &SearchResults::filename() const
{
    return mFilename;
}

void SearchResults::setFilename(const QString &newFilename)
{
    mFilename = newFilename;
}

const QString &SearchResults::folder() const
{
    return mFolder;
}

void SearchResults::setFolder(const QString &newFolder)
{
    mFolder = newFolder;
}

const QString &SearchResults::filters() const
{
    return mFilters;
}

void SearchResults::setFilters(const QString &newFilters)
{
    mFilters = newFilters;
}

bool SearchResults::searchSubfolders() const
{
    return mSearchSubfolders;
}

void SearchResults::setSearchSubfolders(bool newSearchSubfolders)
{
    mSearchSubfolders = newSearchSubfolders;
}

const QList<PSearchResultTreeItem> &SearchResults::results() const
{
    return mResults;
}

PSearchResultTreeItem SearchResults::result(int idx) const
{
    return mResults[idx];
}

const QString &SearchResults::symbolFullname() const
{
    return mSymbolFullname;
}

void SearchResults::setSymbolFullname(const QString &newSymbolFullname)
{
    mSymbolFullname = newSymbolFullname;
}

