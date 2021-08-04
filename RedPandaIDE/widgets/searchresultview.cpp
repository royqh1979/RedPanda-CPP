#include "searchresultview.h"

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
    if (index>0) {
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
    emit modelChanged();
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

}

SearchResultListModel::SearchResultListModel(SearchResultModel *model, QObject *parent):
    QAbstractListModel(parent),
    mSearchResultModel(model)
{
    connect(mSearchResultModel, &SearchResultModel::currentChanged,
            this, &SearchResultListModel::onResultModelChanged);
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
            return tr("Current File") + QString(" \"%1\"").arg(results->keyword);
        case SearchFileScope::wholeProject:
            return tr("Files In Project") + QString(" \"%1\"").arg(results->keyword);
        case SearchFileScope::openedFiles:
            return tr("Open Files") + QString(" \"%1\"").arg(results->keyword);
        }
    }
    return QVariant();
}

void SearchResultListModel::onResultModelChanged()
{
    beginResetModel();
    endResetModel();
}
