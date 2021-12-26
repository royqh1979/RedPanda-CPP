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
    void removeBookmarks(const QString& filename);
    void clear();
    bool updateDescription(const QString&filename, int line, const QString& description);
    void save(const QString& filename);
    void load(const QString& filename);
    void removeBookmarkAt(int i);
public slots:
    void onFileDeleteLines(const QString& filename, int startLine, int count);
    void onFileInsertLines(const QString& filename, int startLine, int count);
private:
    bool isBookmarkExists(const QString&filename, int line);

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
