#ifndef SEARCHRESULTVIEW_H
#define SEARCHRESULTVIEW_H

#include <QTreeView>
#include <QMap>
#include "../qsynedit/SearchBase.h"
#include "utils.h"

#define MAX_SEARCH_RESULTS 20

struct SearchResult {
    QString filename;
    int line;
    int start;
    int len;
};

using PSearchResult = std::shared_ptr<SearchResult>;
using SearchResultList = QList<PSearchResult>;
using PSearchResultList = std::shared_ptr<SearchResultList>;

struct SearchResults{
    SynSearchOptions options;
    QString keyword;
    SearchFileScope scope;
    QMap<QString, PSearchResultList> results;
};

using PSearchResults = std::shared_ptr<SearchResults>;

class SearchResultModel : QObject {
    Q_OBJECT
public:
    explicit SearchResultModel(QObject* parent=nullptr);
    PSearchResults addSearchResults(const QString& keyword,SynSearchOptions options,
                                    SearchFileScope scope);
    PSearchResults results(int index);
    int currentIndex() const;
    int resultsCount() const;
    PSearchResults currentResults();
    void setCurrentIndex(int index);
    void clear();
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
private:
    SearchResultModel *mSearchResultModel;
};

class SearchResultView : public QTreeView
{
    Q_OBJECT
public:
    explicit SearchResultView(QWidget* parent=nullptr);
};

#endif // SEARCHRESULTVIEW_H
