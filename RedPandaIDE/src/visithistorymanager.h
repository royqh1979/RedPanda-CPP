#ifndef VISITHISTORYMANAGER_H
#define VISITHISTORYMANAGER_H

#include <QJsonArray>
#include <QList>
#include <QString>
#include <memory>

struct VisitRecord {
    QString filename;
    qint64 timestamp;
    // last known editor layout (captured when the file was closed)
    int caretX = 0;
    int caretY = 0;
    int topLine = 0;
    int topSegment = 0;
    int left = 0;
    bool wordWrap = false;
};

using PVisitRecord = std::shared_ptr<VisitRecord>;

struct VisitHistory{
    qint64 timestamp;
    QList<PVisitRecord> files;
    QList<PVisitRecord> projects;
};

using PVisitHistory = std::shared_ptr<VisitHistory>;

class VisitHistoryManager
{
public:
    VisitHistoryManager(const QString& filename);

    const QList<PVisitRecord> &files() const;
    const QList<PVisitRecord> &projects() const;
    void clearFiles();
    void clearProjects();
    bool addFile(const QString& filename);
    bool addFile(const VisitRecord& record);
    PVisitRecord findFile(const QString& filename) const;
    void removeFile(const QString& filename);
    bool addProject(const QString& filename);
    void removeProject(const QString& filename);
    void save();
    void load();
private:
    PVisitHistory doLoad(const QString& filename, qint64 criteriaTime);
    QList<PVisitRecord> fromJson(const QJsonArray &array, qint64 criteriaTime);
    void mergeRead(QList<PVisitRecord>& target, const QList<PVisitRecord>& readed);
    QJsonArray toJson(const QList<PVisitRecord>& list);
    bool doAdd(QList<PVisitRecord> &list, PVisitRecord record, int maxCount);
    void doRemove(QList<PVisitRecord> &list, const QString& filename);
private:
    QList<PVisitRecord> mFiles;
    QList<PVisitRecord> mProjects;

    qint64 mLastLoadtime;
    QString mSaveFilename;
    int mMaxFileCount;
    int mMaxProjectCount;
};

#endif // VISITHISTORYMANAGER_H
