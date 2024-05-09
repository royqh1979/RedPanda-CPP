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

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QDateTime>
#include "../utils.h"

BookmarkModel::BookmarkModel(QObject* parent):QAbstractTableModel(parent),
    mIsForProject(false)
{

}

QSet<int> BookmarkModel::bookmarksInFile(const QString &filename, bool forProject)
{
    QSet<int> lines;
    QList<PBookmark> bookmarks;
    if (forProject)
        bookmarks = mProjectBookmarks;
    else
        bookmarks = mBookmarks;
    foreach (const PBookmark& bookmark, bookmarks) {
        if (bookmark->filename.compare(filename, PATH_SENSITIVITY) == 0) {
            lines.insert(bookmark->line);
        }
    }
    return lines;
}

void BookmarkModel::addBookmark(const QString &filename, int line, const QString &description, bool forProject)
{
    Q_ASSERT(!isBookmarkExists(filename,line,forProject));
    PBookmark bookmark = std::make_shared<Bookmark>();
    bookmark->filename = filename;
    bookmark->line = line;
    bookmark->description = description;
    bookmark->timestamp = QDateTime::currentMSecsSinceEpoch();
    if (forProject) {
        if (forProject==mIsForProject)
            beginInsertRows(QModelIndex(),mProjectBookmarks.count(),mProjectBookmarks.count());
        mProjectBookmarks.append(bookmark);
    } else {
        if (forProject==mIsForProject)
            beginInsertRows(QModelIndex(),mBookmarks.count(),mBookmarks.count());
        mBookmarks.append(bookmark);
    }
    if (forProject==mIsForProject)
        endInsertRows();
}

PBookmark BookmarkModel::bookmark(int i, bool forProject)
{
    if (forProject)
        return mProjectBookmarks[i];
    else
        return mBookmarks[i];
}

PBookmark BookmarkModel::bookmark(int i)
{
    return bookmark(i,isForProject());
}

PBookmark BookmarkModel::bookmark(const QString &filename, int line, bool forProject)
{
    QList<PBookmark> bookmarks;
    if (forProject)
        bookmarks = mProjectBookmarks;
    else
        bookmarks = mBookmarks;
    foreach (PBookmark bookmark, bookmarks) {
        if (bookmark->filename.compare(filename, PATH_SENSITIVITY) == 0
                && bookmark->line == line) {
            return bookmark;
        }
    }
    return PBookmark();
}

PBookmark BookmarkModel::bookmark(const QString &filename, int line)
{
    return bookmark(filename,line,isForProject());
}

bool BookmarkModel::removeBookmark(const QString &filename, int line, bool forProject)
{
    QList<PBookmark> bookmarks;
    if (forProject)
        bookmarks = mProjectBookmarks;
    else
        bookmarks = mBookmarks;
    for (int i=0;i<bookmarks.count();i++) {
        PBookmark bookmark = bookmarks[i];
        if (bookmark->filename.compare(filename, PATH_SENSITIVITY) == 0
                && bookmark->line == line) {
            removeBookmarkAt(i, forProject);
            return true;
        }
    }
    return false;
}

void BookmarkModel::removeBookmarks(const QString &filename, bool forProject)
{
    QList<PBookmark> bookmarks;
    if (forProject)
        bookmarks = mProjectBookmarks;
    else
        bookmarks = mBookmarks;
    for (int i=bookmarks.count()-1;i>=0;i--) {
        PBookmark bookmark = bookmarks[i];
        if (bookmark->filename.compare(filename, PATH_SENSITIVITY) == 0) {
            removeBookmarkAt(i, forProject);
        }
    }
}

void BookmarkModel::renameBookmarkFile(const QString& oldFilename, const QString& newFilename, bool forProject)
{
    QList<PBookmark> bookmarks;
    if (forProject)
        bookmarks = mProjectBookmarks;
    else
        bookmarks = mBookmarks;
    for (int i=bookmarks.count()-1;i>=0;i--) {
        PBookmark bookmark = bookmarks[i];
        if (bookmark->filename.compare(oldFilename, PATH_SENSITIVITY) == 0) {
            bookmark->filename=newFilename;
            if (forProject==mIsForProject) {
                QModelIndex index=createIndex(i,2);
                emit dataChanged(index,index);
            }
        }
    }
}

void BookmarkModel::clear(bool forProject)
{
    if (forProject==mIsForProject)
        beginResetModel();
    if (forProject)
        mProjectBookmarks.clear();
    else
        mBookmarks.clear();
    if (forProject==mIsForProject)
        endResetModel();
}

bool BookmarkModel::updateDescription(const QString &filename, int line, const QString &description, bool forProject)
{
    QList<PBookmark> bookmarks;
    if (forProject)
        bookmarks = mProjectBookmarks;
    else
        bookmarks = mBookmarks;
    for (int i=0;i<bookmarks.count();i++) {
        PBookmark bookmark = bookmarks[i];
        if (bookmark->filename.compare(filename, PATH_SENSITIVITY) == 0
                && bookmark->line == line) {
            bookmark->description = description;
            emit dataChanged(createIndex(i,0),createIndex(i,2));
            return true;
        }
    }
    return false;
}

bool BookmarkModel::updateDescription(const QString &filename, int line, const QString &description)
{
    return updateDescription(filename,line,description,mIsForProject);
}

void BookmarkModel::saveBookmarks(const QString &filename)
{
    save(filename,QString());
}

void BookmarkModel::loadBookmarks(const QString &filename)
{
    if (!mIsForProject)
        beginResetModel();
    qint64 t;
    mLastLoadBookmarksTimestamp = QDateTime::currentMSecsSinceEpoch();
    mBookmarks = load(filename,0,&t);
    if (!mIsForProject)
        endResetModel();
}

void BookmarkModel::save(const QString &filename, const QString& projectFolder)
{
    bool forProject = !projectFolder.isEmpty();
    qint64 t,fileTimestamp;
    QHash<QString,int> compareHash;
    QList<PBookmark> &list=forProject?mProjectBookmarks:mBookmarks;
    if (forProject) {
        t=mLastLoadProjectBookmarksTimestamp;
    } else {
        t=mLastLoadBookmarksTimestamp;
    }
    for (int i=0;i<list.count();i++) {
        const PBookmark& bookmark=list[i];
        QString filename = forProject?extractRelativePath(projectFolder, bookmark->filename):bookmark->filename;
        QString key = QString("%1-%2").arg(filename).arg(bookmark->line);
        compareHash.insert(key,i);
    }
    QList<PBookmark> fileBookmarks;
    try {
        fileBookmarks=load(filename, t,&fileTimestamp);
    } catch (FileError& e) {

    }

    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {        

//        QList<PBookmark> saveBookmarks;

        QDir dir(projectFolder);
        foreach (const PBookmark& bookmark, fileBookmarks) {
            QString key = QString("%1-%2").arg(bookmark->filename).arg(bookmark->line);
            int idx = compareHash.value(key,-1);
            if (idx<0) {
                int count=list.count();
                compareHash.insert(key,count);
                if (forProject == mIsForProject) {
                    beginInsertRows(QModelIndex(),count,count);
                }
                if (forProject) {
                    bookmark->filename = cleanPath(dir.absoluteFilePath(bookmark->filename));
                }

                list.append(bookmark);
                if (forProject == mIsForProject)
                    endInsertRows();
            } else {
                PBookmark pTemp = list[idx];
                if (pTemp->timestamp<=bookmark->timestamp) {
                    bookmark->description = pTemp->description;
                    bookmark->timestamp = pTemp->timestamp;
                    if (forProject == mIsForProject)
                        emit dataChanged(createIndex(idx,2),createIndex(idx,2));
                }
            }
        }

        qint64 saveTime = QDateTime::currentMSecsSinceEpoch();;
        if (forProject) {
            mLastLoadProjectBookmarksTimestamp = saveTime;
        } else {
            mLastLoadBookmarksTimestamp = saveTime;
        }

        QJsonObject rootObj;
        rootObj["timestamp"]=QString("%1").arg(saveTime);
        QJsonArray array;
        foreach (const PBookmark& bookmark, list) {
            QJsonObject obj;
            QString filename = forProject?extractRelativePath(projectFolder, bookmark->filename):bookmark->filename;
            obj["filename"]=filename;
            obj["line"]=bookmark->line;
            obj["description"]=bookmark->description;
            obj["timestamp"]=QString("%1").arg(bookmark->timestamp);
            array.append(obj);
        }
        rootObj["bookmarks"]=array;
        QJsonDocument doc;
        doc.setObject(rootObj);
        if (file.write(doc.toJson())<0) {
            throw FileError(tr("Save file '%1' failed.")
                            .arg(filename));
        }
    } else {
        throw FileError(tr("Can't open file '%1' for write.")
                        .arg(filename));
    }
}

QList<PBookmark> BookmarkModel::load(const QString& filename, qint64 criteriaTimestamp, qint64* pFileTimestamp)
{
    //clear(forProject);
    QList<PBookmark> bookmarks;
    QFileInfo fileInfo(filename);
    QFile file(filename);
    if (!file.exists())
        return bookmarks;
    if (file.open(QFile::ReadOnly)) {
        QByteArray content = file.readAll().trimmed();
        if (content.isEmpty())
            return bookmarks;
        QJsonParseError error;
        QJsonDocument doc(QJsonDocument::fromJson(content,&error));
        if (error.error  != QJsonParseError::NoError) {
            throw FileError(tr("Error in json file '%1':%2 : %3")
                            .arg(filename)
                            .arg(error.offset)
                            .arg(error.errorString()));
        }
        QJsonObject rootObj=doc.object();
        bool ok;
        qint64 timestamp = rootObj["timestamp"].toString().toLongLong(&ok);
        if (!ok || timestamp<=criteriaTimestamp)
            return bookmarks;
        *pFileTimestamp=timestamp;
        QJsonArray array = rootObj["bookmarks"].toArray();
        for  (int i=0;i<array.count();i++) {
            QJsonValue value = array[i];
            QJsonObject obj=value.toObject();
            qint64 bookmarkTimestamp = obj["timestamp"].toString().toULongLong(&ok);
            if (ok && bookmarkTimestamp>criteriaTimestamp) {
                PBookmark bookmark = std::make_shared<Bookmark>();
                bookmark->filename = obj["filename"].toString();
                bookmark->line = obj["line"].toInt();
                bookmark->description = obj["description"].toString();
                bookmark->timestamp=obj["timestamp"].toString().toULongLong();
                bookmarks.append(bookmark);
            }
        }
    } else {
        throw FileError(tr("Can't open file '%1' for read.")
                        .arg(filename));
    }
    return bookmarks;
}

void BookmarkModel::saveProjectBookmarks(const QString &filename, const QString& projectFolder)
{
    save(filename,projectFolder);
}

void BookmarkModel::loadProjectBookmarks(const QString &filename, const QString& projectFolder)
{
    if (mIsForProject)
        beginResetModel();
    qint64 t;
    mLastLoadProjectBookmarksTimestamp = QDateTime::currentMSecsSinceEpoch();
    mProjectBookmarks = load(filename,0,&t);
    QDir folder(projectFolder);
    foreach (PBookmark bookmark, mProjectBookmarks) {
        bookmark->filename=cleanPath(folder.absoluteFilePath(bookmark->filename));
    }
    if (mIsForProject)
        endResetModel();
}

void BookmarkModel::onFileDeleteLines(const QString &filename, int startLine, int count, bool forProject)
{
    QList<PBookmark> bookmarks;
    if (forProject)
        bookmarks = mProjectBookmarks;
    else
        bookmarks = mBookmarks;
    for (int i = bookmarks.count()-1;i>=0;i--){
        PBookmark bookmark = bookmarks[i];
        if  (bookmark->filename == filename
             && bookmark->line>=startLine) {
            if (bookmark->line >= startLine+count) {
                bookmark->line -= count;
                if (forProject == mIsForProject)
                    emit dataChanged(createIndex(i,0),createIndex(i,2));
            } else {
                removeBookmarkAt(i,forProject);
            }
        }
    }
}

void BookmarkModel::onFileInsertLines(const QString &filename, int startLine, int count, bool forProject)
{
    QList<PBookmark> bookmarks;
    if (forProject)
        bookmarks = mProjectBookmarks;
    else
        bookmarks = mBookmarks;
    for (int i = bookmarks.count()-1;i>=0;i--){
        PBookmark bookmark = bookmarks[i];
        if  (bookmark->filename == filename
             && bookmark->line>=startLine) {
            bookmark->line+=count;
            if (forProject == mIsForProject)
                emit dataChanged(createIndex(i,0),createIndex(i,2));
        }
    }
}

void BookmarkModel::removeBookmarkAt(int i, bool forProject)
{
    if (forProject == mIsForProject)
        beginRemoveRows(QModelIndex(), i,i);
    if (forProject)
        mProjectBookmarks.removeAt(i);
    else
        mBookmarks.removeAt(i);
    if (forProject == mIsForProject)
        endRemoveRows();
}

void BookmarkModel::removeBookmarkAt(int i)
{
    return removeBookmarkAt(i,isForProject());
}

bool BookmarkModel::isBookmarkExists(const QString &filename, int line, bool forProject)
{
    QList<PBookmark> bookmarks;
    if (forProject)
        bookmarks = mProjectBookmarks;
    else
        bookmarks = mBookmarks;
    foreach (const PBookmark& bookmark, bookmarks) {
        if (bookmark->filename.compare(filename, PATH_SENSITIVITY) == 0
                && bookmark->line == line) {
            return true;
        }
    }
    return false;
}

bool BookmarkModel::isForProject() const
{
    return mIsForProject;
}

void BookmarkModel::setIsForProject(bool newIsForProject)
{
    if (newIsForProject!=mIsForProject) {
        mIsForProject = newIsForProject;
        beginResetModel();
        endResetModel();
    }
}

void BookmarkModel::sort(int column, Qt::SortOrder order)
{
    switch(column) {
    case 0:
        if (order == Qt::SortOrder::AscendingOrder) {
            auto sorter=[](PBookmark b1,PBookmark b2) {
                return b1->description < b2->description;
            };
            std::sort(mBookmarks.begin(),mBookmarks.end(),sorter);
            std::sort(mProjectBookmarks.begin(),mProjectBookmarks.end(),sorter);
        } else {
            auto sorter=[](PBookmark b1,PBookmark b2) {
                return b2->description<b1->description;
            };
            std::sort(mBookmarks.begin(),mBookmarks.end(),sorter);
            std::sort(mProjectBookmarks.begin(),mProjectBookmarks.end(),sorter);
        }
        break;
    case 1:
        if (order == Qt::SortOrder::AscendingOrder) {
            auto sorter=[](PBookmark b1,PBookmark b2) {
                return b1->filename<b2->filename;
            };
            std::sort(mBookmarks.begin(),mBookmarks.end(),sorter);
            std::sort(mProjectBookmarks.begin(),mProjectBookmarks.end(),sorter);
        } else {
            auto sorter=[](PBookmark b1,PBookmark b2) {
                return b2->filename<b1->filename;
            };
            std::sort(mBookmarks.begin(),mBookmarks.end(),sorter);
            std::sort(mProjectBookmarks.begin(),mProjectBookmarks.end(),sorter);
        }
        break;
    case 2:
        if (order == Qt::SortOrder::AscendingOrder) {
            auto sorter=[](PBookmark b1,PBookmark b2) {
                return b1->line<b2->line;
            };
            std::sort(mBookmarks.begin(),mBookmarks.end(),sorter);
            std::sort(mProjectBookmarks.begin(),mProjectBookmarks.end(),sorter);
        } else {
            auto sorter=[](PBookmark b1,PBookmark b2) {
                return b2->line<b1->line;
            };
            std::sort(mBookmarks.begin(),mBookmarks.end(),sorter);
            std::sort(mProjectBookmarks.begin(),mProjectBookmarks.end(),sorter);
        }
        break;
    }
}

int BookmarkModel::rowCount(const QModelIndex &) const
{
    if (mIsForProject)
        return mProjectBookmarks.count();
    else
        return mBookmarks.count();
}

QVariant BookmarkModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    int row = index.row();
    PBookmark bookmark;
    if (mIsForProject)
        bookmark = mProjectBookmarks[row];
    else
        bookmark = mBookmarks[row];
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

