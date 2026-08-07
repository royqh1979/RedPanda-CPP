#ifndef DIRMODEL_H
#define DIRMODEL_H

#include <QAbstractListModel>
#include <QDateTime>
#include <QFileInfo>
#include <QFileInfoList>
#include <QObject>
#include <QPixmap>
#include <QRecursiveMutex>
#include <QSet>
#include <QThread>
#include <memory>

struct ImageInfo {
    bool isDir;
    QString filename;
    QString fullPath;
    QPixmap thumbnail;
    QDateTime thumbnailTime;
};

using PImageInfo = std::shared_ptr<ImageInfo>;

class DirModel : public QAbstractListModel
{
    Q_OBJECT
public:
    explicit DirModel(QObject *parent = nullptr);
    void open(const QString& path);
    void close();
    const QString &path() const;
    void setCurrentFileIdx(int fileIdx);
    int currentFileIdx() const;
    void toNext();
    void toPrevious();
    void toFirst();
    void toLast();
    int imageCount() const;
    QString imageFileName(int idx) const;
    QString imagePath(int idx) const;
    QPixmap thumbnail(int idx) const;
    int thumbnailSize() const;
    void setThumbnailSize(int newThumbnailSize);
signals:
    void currentFileChanged(int oldFileId, int currentFileId);
    void pathChanged(const QString& oldPath, const QString& newPath);
private slots:
    void setThumbnail(const QString &dirPath, int imageIdx, int thumbnailSize, const QString &imagePath, QPixmap thumbnail);
private:
    void clearThumbnails();
    void loadThumbnail(int idx, const QString& path) const;
    void reloadDirThumbnail();
private:
    QString mPath;
    int mThumbnailSize;
    QList<PImageInfo> mImageInfos;
    QHash<QString, PImageInfo> mImageInfoIndex;
    int mSubDirCount;
    int mCurrentFileIdx;
    QPixmap mDirThumbnail;
    mutable QSet<int> mLoadings;
    mutable QRecursiveMutex mMutex;

    // QAbstractItemModel interface
public:
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDragActions() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    int subDirCount() const;
};

class ThumbnailLoader: public QThread {
    Q_OBJECT
public:
    ThumbnailLoader(const QString &dirPath, int imageIdx, int thumbnailSize, const QString &imagePath, QObject* parent=nullptr);
signals:
    void thumbnailLoaded(const QString &dirPath, int imageIdx,int thumbnailSize, const QString &imagePath, const QPixmap &thumbnail);
private:
    QString mDirPath;
    int mImageIdx;
    QString mImagePath;
    int mThumbnailSize;

    // QThread interface
protected:
    void run() override;
};

#endif // DIRMODEL_H
