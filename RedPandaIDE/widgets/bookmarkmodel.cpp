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
#include "bookmarkmodel.h"
#include "../systemconsts.h"

#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include "../utils.h"

BookmarkModel::BookmarkModel(QObject* parent):QAbstractTableModel(parent)
{

}

QSet<int> BookmarkModel::bookmarksInFile(const QString &filename)
{
    QSet<int> lines;
    foreach (const PBookmark& bookmark, mBookmarks) {
        if (bookmark->filename.compare(filename, PATH_SENSITIVITY) == 0) {
            lines.insert(bookmark->line);
        }
    }
    return lines;
}

void BookmarkModel::addBookmark(const QString &filename, int line, const QString &description)
{
    Q_ASSERT(!isBookmarkExists(filename,line));
    PBookmark bookmark = std::make_shared<Bookmark>();
    bookmark->filename = filename;
    bookmark->line = line;
    bookmark->description = description;
    beginInsertRows(QModelIndex(),mBookmarks.count(),mBookmarks.count());
    mBookmarks.append(bookmark);
    endInsertRows();
}

PBookmark BookmarkModel::bookmark(int i)
{
    return mBookmarks[i];
}

PBookmark BookmarkModel::bookmark(const QString &filename, int line)
{
    for (int i=0;i<mBookmarks.count();i++) {
        PBookmark bookmark = mBookmarks[i];
        if (bookmark->filename.compare(filename, PATH_SENSITIVITY) == 0
                && bookmark->line == line) {
            return bookmark;
        }
    }
    return PBookmark();
}

bool BookmarkModel::removeBookmark(const QString &filename, int line)
{
    for (int i=0;i<mBookmarks.count();i++) {
        PBookmark bookmark = mBookmarks[i];
        if (bookmark->filename.compare(filename, PATH_SENSITIVITY) == 0
                && bookmark->line == line) {
            removeBookmarkAt(i);
            return true;
        }
    }
    return false;
}

void BookmarkModel::removeBookmarks(const QString &filename)
{
    for (int i=mBookmarks.count()-1;i>=0;i--) {
        PBookmark bookmark = mBookmarks[i];
        if (bookmark->filename.compare(filename, PATH_SENSITIVITY) == 0) {
            removeBookmarkAt(i);
        }
    }
}

void BookmarkModel::clear()
{
    beginResetModel();
    mBookmarks.clear();
    endResetModel();
}

bool BookmarkModel::updateDescription(const QString &filename, int line, const QString &description)
{
    for (int i=0;i<mBookmarks.count();i++) {
        PBookmark bookmark = mBookmarks[i];
        if (bookmark->filename.compare(filename, PATH_SENSITIVITY) == 0
                && bookmark->line == line) {
            bookmark->description = description;
            emit dataChanged(createIndex(i,0),createIndex(i,2));
            return true;
        }
    }
    return false;
}

void BookmarkModel::save(const QString &filename)
{
    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QJsonArray array;
        foreach (const PBookmark& bookmark, mBookmarks) {
            QJsonObject obj;
            obj["filename"]=bookmark->filename;
            obj["line"]=bookmark->line;
            obj["description"]=bookmark->description;
            array.append(obj);
        }
        QJsonDocument doc;
        doc.setArray(array);
        if (file.write(doc.toJson())<0) {
            throw FileError(tr("Save file '%1' failed.")
                            .arg(filename));
        }
    } else {
        throw FileError(tr("Can't open file '%1' for write.")
                        .arg(filename));
    }
}

void BookmarkModel::load(const QString& filename)
{
    clear();
    QFile file(filename);
    if (!file.exists())
        return;
    if (file.open(QFile::ReadOnly)) {
        QByteArray content = file.readAll();
        QJsonParseError error;
        QJsonDocument doc(QJsonDocument::fromJson(content,&error));
        if (error.error  != QJsonParseError::NoError) {
            throw FileError(tr("Error in json file '%1':%2 : %3")
                            .arg(filename)
                            .arg(error.offset)
                            .arg(error.errorString()));
        }
        QJsonArray array = doc.array();
        for  (int i=0;i<array.count();i++) {
            QJsonValue value = array[i];
            QJsonObject obj=value.toObject();
            addBookmark( QFileInfo(obj["filename"].toString()).absoluteFilePath(),
                    obj["line"].toInt(),
                    obj["description"].toString());

        }
    } else {
        throw FileError(tr("Can't open file '%1' for read.")
                        .arg(filename));
    }
}

void BookmarkModel::onFileDeleteLines(const QString &filename, int startLine, int count)
{
    for (int i = mBookmarks.count()-1;i>=0;i--){
        PBookmark bookmark = mBookmarks[i];
        if  (bookmark->filename == filename
             && bookmark->line>=startLine) {
            if (bookmark->line >= startLine+count) {
                bookmark->line -= count;
                emit dataChanged(createIndex(i,0),createIndex(i,2));
            } else {
                removeBookmarkAt(i);
            }
        }
    }
}

void BookmarkModel::onFileInsertLines(const QString &filename, int startLine, int count)
{
    for (int i = mBookmarks.count()-1;i>=0;i--){
        PBookmark bookmark = mBookmarks[i];
        if  (bookmark->filename == filename
             && bookmark->line>=startLine) {
            bookmark->line+=count;
            emit dataChanged(createIndex(i,0),createIndex(i,2));
        }
    }
}

void BookmarkModel::removeBookmarkAt(int i)
{
    beginRemoveRows(QModelIndex(), i,i);
    mBookmarks.removeAt(i);
    endRemoveRows();
}

bool BookmarkModel::isBookmarkExists(const QString &filename, int line)
{
    foreach (const PBookmark& bookmark, mBookmarks) {
        if (bookmark->filename.compare(filename, PATH_SENSITIVITY) == 0
                && bookmark->line == line) {
            return true;
        }
    }
    return false;
}

int BookmarkModel::rowCount(const QModelIndex &) const
{
    return mBookmarks.count();
}

QVariant BookmarkModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    int row = index.row();
    PBookmark bookmark = mBookmarks[row];
    if (role == Qt::DisplayRole) {
        switch(index.column()) {
        case 0:
            return bookmark->description;
        case 1:
            return bookmark->line;
        case 2:
            return bookmark->filename;
        }
    }
    return QVariant();
}

int BookmarkModel::columnCount(const QModelIndex &) const
{
    return 3;
}

QVariant BookmarkModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal) {
        if (role == Qt::DisplayRole) {
            switch(section) {
            case 0:
                return tr("Description");
            case 1:
                return tr("Line");
            case 2:
                return tr("Filename");
            }
        }
    }
    return QVariant();
}

