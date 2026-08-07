#include "dirmodel.h"

#include <QDir>
#include <QImageReader>
#include <QMimeData>
#include <QSemaphore>
#include <QApplication>
#include <QStyle>


DirModel::DirModel(QObject *parent) : QAbstractListModel(parent),
    mThumbnailSize{256},
    mCurrentFileIdx{-1}
{
    reloadDirThumbnail();
}

void DirModel::open(const QString &path)
{
    QString oldPath = mPath;
    QFileInfo info{path};
    if (!info.exists())
        return;
    QDir dir;
    QString fileName;
    if (info.isDir()) {
        dir = QDir{path};
    } else {
        dir = info.dir();
        fileName = info.fileName();
    }
    mPath = dir.absolutePath();
    if (oldPath != mPath)
        mCurrentFileIdx = -1;

    QFileInfoList fileInfos =  dir.entryInfoList(QDir::Filter::Files );
    QFileInfoList dirInfos =  dir.entryInfoList(QDir::Filter::Dirs);
    beginResetModel();
    QMutexLocker lock{&mMutex};
    mLoadings.clear();
    mImageInfos.clear();
    mImageInfoIndex.clear();
    mSubDirCount = 0;
    int idx = -1;
    foreach(const QFileInfo & dirInfo, dirInfos) {
        if (dirInfo.fileName()!=".") {
            PImageInfo info = std::make_shared<ImageInfo>();
            info->filename = dirInfo.fileName();
            info->isDir=true;
            info->fullPath = dir.absoluteFilePath(info->filename);
            info->thumbnail = mDirThumbnail;
            info->thumbnailTime = QDateTime::currentDateTime();
            mImageInfos.append(info);
            mImageInfoIndex.insert(info->fullPath,info);
        }
    }
    mSubDirCount = mImageInfos.count();
    foreach(const QFileInfo & fileInfo, fileInfos) {
        if (QImageReader::supportedImageFormats().contains(fileInfo.suffix().toLower().toLocal8Bit())) {
            PImageInfo info = std::make_shared<ImageInfo>();
            info->filename = fileInfo.fileName();
            info->isDir=false;
            info->fullPath = dir.absoluteFilePath(info->filename);
            info->thumbnailTime = QDateTime::currentDateTime();
            if (info->filename == fileName)
                idx = mImageInfos.count();
            mImageInfos.append(info);
            mImageInfoIndex.insert(info->fullPath,info);
        }
    }
    endResetModel();
    emit pathChanged(oldPath, mPath);
    if (mImageInfos.isEmpty())
        mCurrentFileIdx = -1;
    else {
        if (idx == -1)
            toFirst();
        else
            setCurrentFileIdx(idx);
    }
}

void DirModel::close()
{
    if (mImageInfos.count()==0)
        return;
    QString oldPath = mPath;
    int oldIdx = mCurrentFileIdx;
    mPath = "";
    beginResetModel();
    QMutexLocker lock{&mMutex};
    mLoadings.clear();
    mImageInfos.clear();
    mImageInfoIndex.clear();
    mSubDirCount = 0;
    mCurrentFileIdx = -1;
    endResetModel();
    emit pathChanged(oldPath, mPath);
    emit currentFileChanged(oldIdx,mCurrentFileIdx);
}

const QString &DirModel::path() const
{
    return mPath;
}

void DirModel::setCurrentFileIdx(int fileIdx)
{
    if (fileIdx < 0)
        fileIdx = 0;
    if (fileIdx >= imageCount())
        fileIdx = imageCount()-1;
    if (fileIdx == mCurrentFileIdx)
        return;
    int oldIdx = mCurrentFileIdx;
    mCurrentFileIdx = fileIdx;
    emit currentFileChanged(oldIdx, mCurrentFileIdx);
}

int DirModel::currentFileIdx() const
{
    return mCurrentFileIdx;
}

void DirModel::toNext()
{
    int idx = currentFileIdx()+1;
    if (idx >= imageCount())
        idx = mSubDirCount;
    setCurrentFileIdx(idx);
}

void DirModel::toPrevious()
{
    int idx = currentFileIdx()-1;
    if (idx < mSubDirCount)
        idx = imageCount()-1;
    setCurrentFileIdx(idx);
}

void DirModel::toFirst()
{
    setCurrentFileIdx(mSubDirCount);
}

void DirModel::toLast()
{
    setCurrentFileIdx(imageCount()-1);
}

int DirModel::imageCount() const
{
    QMutexLocker locker{&mMutex};
    return mImageInfos.count();
}

QString DirModel::imageFileName(int idx) const
{
    QMutexLocker locker{&mMutex};
    if (idx<0 || idx>=mImageInfoIndex.count())
        return QString();
    return mImageInfos[idx]->filename;
}

QString DirModel::imagePath(int idx) const
{
    QMutexLocker locker{&mMutex};
    if (idx<0 || idx>=mImageInfoIndex.count())
        return QString();
    return mImageInfos[idx]->fullPath;
}

QPixmap DirModel::thumbnail(int idx) const
{
    QMutexLocker locker{&mMutex};
    QPixmap defaultThumb;
    if (idx<0 || idx>=mImageInfoIndex.count())
        return defaultThumb;
    if (mImageInfos[idx]->thumbnail.isNull()) {
        loadThumbnail(idx,mImageInfos[idx]->fullPath);
        return defaultThumb;
    }
    return mImageInfos[idx]->thumbnail;
}

void DirModel::setThumbnail(const QString &dirPath, int imageIdx, int thumbnailSize, const QString &imagePath, QPixmap thumbnail)
{
    QMutexLocker locker(&mMutex);
    mLoadings.remove(imageIdx);
    if (mPath == dirPath && mThumbnailSize == thumbnailSize && imageIdx>=0){
        if (imageIdx < mImageInfos.count()) {
            PImageInfo info = mImageInfos[imageIdx];
            if (info->fullPath == imagePath) {
                info->thumbnail = thumbnail;
                info->thumbnailTime = QDateTime::currentDateTime();
                QModelIndex index = createIndex(imageIdx,0);
                emit dataChanged(index,index);
            }
        }
    }
}

void DirModel::clearThumbnails()
{
    QMutexLocker locker{&mMutex};
    reloadDirThumbnail();
    foreach(PImageInfo info, mImageInfos) {
        if (info->isDir)
            info->thumbnail = mDirThumbnail;
        else
            info->thumbnail = QPixmap();
    }
    QModelIndex top=createIndex(0,0);
    QModelIndex bottom = createIndex(mImageInfos.count()-1,0);
    emit dataChanged(top,bottom);
}

void DirModel::loadThumbnail(int idx, const QString &path) const
{
    QMutexLocker locker{&mMutex};
    if (mLoadings.contains(idx))
        return;
    mLoadings.insert(idx);
    ThumbnailLoader *loader=new ThumbnailLoader(mPath,idx,mThumbnailSize,path);
    connect(loader, &ThumbnailLoader::thumbnailLoaded,
            this, &DirModel::setThumbnail);
    loader->start();
}

void DirModel::reloadDirThumbnail()
{
    QPixmap thumb{":/icons/images/glyphs-poly--folder.svg"};
    mDirThumbnail = thumb.scaled(mThumbnailSize,mThumbnailSize);
}

int DirModel::subDirCount() const
{
    return mSubDirCount;
}

int DirModel::thumbnailSize() const
{
    return mThumbnailSize;
}

void DirModel::setThumbnailSize(int newThumbnailSize)
{
    if (newThumbnailSize!=mThumbnailSize) {
        mThumbnailSize = newThumbnailSize;
        clearThumbnails();
    }
}

QStringList DirModel::mimeTypes() const
{
    return QStringList{"text/uri-list"};
}

QMimeData *DirModel::mimeData(const QModelIndexList &indexes) const
{
    QMimeData *mime = new QMimeData();
    QDir dir{mPath};
    if (dir.exists()) {
        QMutexLocker locker{&mMutex};
        QStringList texts;
        QList<QUrl> urls;
        for (const QModelIndex &idx : indexes) {
            QString path = mImageInfos[idx.row()]->fullPath;
            texts << path;
            urls.append(QUrl::fromLocalFile(path));
        }
        mime->setUrls(urls);
        mime->setText(texts.join("\n"));
    }
    return mime;
}

Qt::ItemFlags DirModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) return Qt::NoItemFlags;
    return QAbstractListModel::flags(index) | Qt::ItemIsDragEnabled;
}

Qt::DropActions DirModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

int DirModel::rowCount(const QModelIndex &parent) const
{
    return mImageInfos.count();
}

QVariant DirModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    int row =index.row();
    if (row<0 || row>=imageCount())
        return QVariant();
    switch(role) {
    case Qt::ToolTipRole:
        return imagePath(row);
    case Qt::DisplayRole:
        return imageFileName(row);
    case Qt::DecorationRole:
        return thumbnail(row);
    }
    return QVariant();
}

static QSemaphore thumbnailLoaderSemaphore{5};
ThumbnailLoader::ThumbnailLoader(const QString &dirPath, int imageIdx, int thumbnailSize, const QString &imagePath, QObject *parent)
{
    mDirPath = dirPath;
    mImageIdx = imageIdx;
    mThumbnailSize = thumbnailSize;
    mImagePath = imagePath;
    connect(this, &QThread::finished,
            this, &QObject::deleteLater);
}

void ThumbnailLoader::run()
{
    QPixmap pixmap;
    int delayCount = 0;
    while (true) {
        thumbnailLoaderSemaphore.acquire(1);
        QImageReader reader{mImagePath};
        reader.setAutoTransform(true);
        QImage image = reader.read();
        thumbnailLoaderSemaphore.release(1);
        if (!image.isNull()) {
            pixmap = QPixmap::fromImage(image);
            break;
        }
        delayCount++;
        if (delayCount>10) {
            return;
        }
        QThread::msleep(100);
    }
    QPixmap thumbnail;
    if (pixmap.width()>pixmap.height()) {
        thumbnail = pixmap.scaledToWidth(mThumbnailSize);
    } else {
        thumbnail = pixmap.scaledToHeight(mThumbnailSize);
    }
    emit thumbnailLoaded(mDirPath, mImageIdx, mThumbnailSize, mImagePath, thumbnail);
}
