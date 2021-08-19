#include "searchresultview.h"
#include <QApplication>
#include <QPainter>
#include <QStyledItemDelegate>
#include "mainwindow.h"

PSearchResults SearchResultModel::addSearchResults(const QString &keyword, SynSearchOptions options, SearchFileScope scope)
{
    int index=-1;
    for (int i=0;i<mSearchResults.size();i++) {
        PSearchResults results = mSearchResults[i];
        if (results->keyword == keyword && results->scope == scope) {
            index=i;
            break;
        }
    }
    if (index>=0) {
        mSearchResults.removeAt(index);
    }
    if (mSearchResults.size()>=MAX_SEARCH_RESULTS) {
        mSearchResults.pop_back();
    }
    PSearchResults results = std::make_shared<SearchResults>();
    results->keyword = keyword;
    results->options = options;
    results->scope = scope;
    mSearchResults.push_front(results);
    mCurrentIndex = 0;
    return results;
}

PSearchResults SearchResultModel::results(int index)
{
    if (index<0 || index>=mSearchResults.size()) {
        return PSearchResults();
    }
    return mSearchResults[index];
}

void SearchResultModel::notifySearchResultsUpdated()
{
    emit modelChanged();
}

SearchResultModel::SearchResultModel(QObject* parent):
    QObject(parent),
    mCurrentIndex(-1)
{

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

void SearchResultModel::setCurrentIndex(int index)
{
    if (index!=mCurrentIndex &&
            index>=0 && index<mSearchResults.size()) {
        mCurrentIndex = index;
        emit currentChanged(mCurrentIndex);
    }
}

void SearchResultModel::clear()
{
    mCurrentIndex = -1;
    mSearchResults.clear();
    emit modelChanged();
}

SearchResultTreeModel::SearchResultTreeModel(SearchResultModel *model, QObject *parent):
    QAbstractItemModel(parent),
    mSearchResultModel(model)
{
    connect(mSearchResultModel,&SearchResultModel::currentChanged,
            this,&SearchResultTreeModel::onResultModelChanged);
    connect(mSearchResultModel,&SearchResultModel::modelChanged,
            this,&SearchResultTreeModel::onResultModelChanged);
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
        childItem = results->results[row];
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
        return searchResults->results.count();
    }
    SearchResultTreeItem* item = static_cast<SearchResultTreeItem *>(parent.internalPointer());    if (!item)
        return 0;
    return item->results.count();
}

int SearchResultTreeModel::columnCount(const QModelIndex &parent) const
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
             return item->filename;
         } else {
             return QString("%1 %2: %3").arg(tr("Line")).arg(item->line)
                 .arg(item->text);
         }
    }
    return QVariant();

}

SearchResultModel *SearchResultTreeModel::searchResultModel() const
{
    return mSearchResultModel;
}

void SearchResultTreeModel::onResultModelChanged()
{
    beginResetModel();
    endResetModel();
}

SearchResultListModel::SearchResultListModel(SearchResultModel *model, QObject *parent):
    QAbstractListModel(parent),
    mSearchResultModel(model)
{
    connect(mSearchResultModel, &SearchResultModel::modelChanged,
            this, &SearchResultListModel::onResultModelChanged);
}

int SearchResultListModel::rowCount(const QModelIndex &parent) const
{
    return mSearchResultModel->resultsCount();
}

QVariant SearchResultListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole) {
        PSearchResults results = mSearchResultModel->results(index.row());
        if (!results)
            return QVariant();
        switch (results->scope) {
        case SearchFileScope::currentFile:
            return tr("Current File:") + QString(" \"%1\"").arg(results->keyword);
        case SearchFileScope::wholeProject:
            return tr("Files In Project:") + QString(" \"%1\"").arg(results->keyword);
        case SearchFileScope::openedFiles:
            return tr("Open Files:") + QString(" \"%1\"").arg(results->keyword);
        }
    }
    return QVariant();
}

void SearchResultListModel::onResultModelChanged()
{
    beginResetModel();
    endResetModel();
}

/**
 *
 * see https://stackoverflow.com/questions/1956542/how-to-make-item-view-render-rich-html-text-in-qt/66412883#66412883
 */
SearchResultTreeViewDelegate::SearchResultTreeViewDelegate(PSearchResultTreeModel model, QObject *parent):
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
     option->text = QString();
     style->drawControl(QStyle::CE_ItemViewItem, &option, painter, option.widget);
     SearchResultTreeItem* item = static_cast<SearchResultTreeItem *>(index.internalPointer());

     QString fullText;
     if (item->parent==nullptr) { //is filename
         fullText = item->filename;
     } else {
         fullText = QString("%1 %2: %3").arg(tr("Line")).arg(item->line)
             .arg(item->text);
     }
     // Figure out where to render the text in order to follow the requested alignment
     option->text = fullText;
     QRect textRect = style->subElementRect(QStyle::SE_ItemViewItemText, &option);

     QFontMetrics metrics = option.fontMetrics;
     int x=textRect.left();
     int y=textRect.top() + metrics.ascent();
     if (item->parent==nullptr) { //is filename
        painter->drawText(x,y,fullText);
     } else {
         QString s = item->text.mid(0,item->start-1);
         QString text = QString("%1 %2: %3").arg(tr("Line")).arg(item->line)
                 .arg(s);
         painter->drawText(x,y,text);
         x+=metrics.horizontalAdvance(text);
         QFont font = option.font;
         font.setBold(true);
         font.setItalic(true);
         QFont oldFont = painter->font();
         painter->setFont(font);
         text=item->text.mid(item->start-1,item->len);
         painter->drawText(x,y,text);
         metrics = QFontMetrics(font);
         x+=metrics.horizontalAdvance(text);
         painter->setFont(oldFont);
         text = item->text.mid(item->start-1+item->len);
         painter->drawText(x,y,text);
     }


}
