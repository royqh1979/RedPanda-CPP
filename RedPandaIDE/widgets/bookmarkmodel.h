#ifndef BOOKMARKMODEL_H
#define BOOKMARKMODEL_H

#include <QAbstractTableModel>
#include <memory>
#include <QDebug>

struct Bookmark {
    QString filename;
    int line;
    QString description;
};

using PBookmark=std::shared_ptr<Bookmark>;

class BookmarkModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    BookmarkModel(QObject* parent=nullptr);
    QSet<int> bookmarksInFile(const QString& filename);
    void addBookmark(const QString&filename, int line, const QString& description);
    PBookmark bookmark(int i);
    PBookmark bookmark(const QString&filename, int line);
    bool removeBookmark(const QString&filename, int line);
    void clear();
    bool updateDescription(const QString&filename, int line, const QString& description);
    void save(const QString& filename);
    void load(const QString& filename);
    void removeBookmarkAt(int i);
public slots:
    void onFileDeleteLines(const QString& filename, int startLine, int count);
    void onFileInsertLines(const QString& filename, int startLine, int count);
private:
#ifdef QT_DEBUG
    bool isBookmarkExists(const QString&filename, int line);
#endif
private:
    QList<PBookmark> mBookmarks;

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int columnCount(const QModelIndex &parent) const override;

    // QAbstractItemModel interface
public:
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
};

using PBookmarkModel = std::shared_ptr<BookmarkModel>;

#endif // BOOKMARKMODEL_H
