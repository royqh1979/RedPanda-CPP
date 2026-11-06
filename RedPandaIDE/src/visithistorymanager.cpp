#include "visithistorymanager.h"

#include <QDateTime>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QSet>

VisitHistoryManager::VisitHistoryManager(const QString& filename):
    mLastLoadtime(0),
    mSaveFilename(filename),
    mMaxFileCount(15),
    mMaxProjectCount(15)
{

}

const QList<PVisitRecord> &VisitHistoryManager::files() const
{
    return mFiles;
}

const QList<PVisitRecord> &VisitHistoryManager::projects() const
{
    return mProjects;
}

void VisitHistoryManager::clearFiles()
{
    mFiles.clear();
    save();
}

void VisitHistoryManager::clearProjects()
{
    mProjects.clear();
    save();
}

static int indexOf(const QList<PVisitRecord> &list, const QString& filename) {
    for (int i=0;i<list.count();i++) {
        if (list[i]->filename == filename)
            return i;
    }
    return -1;
}

bool VisitHistoryManager::addFile(const QString &filename)
{
    PVisitRecord record = std::make_shared<VisitRecord>();
    record->filename = filename;
    return doAdd(mFiles,record,mMaxFileCount);
}

bool VisitHistoryManager::addFile(const VisitRecord &record)
{
    return doAdd(mFiles,std::make_shared<VisitRecord>(record),mMaxFileCount);
}

PVisitRecord VisitHistoryManager::findFile(const QString &filename) const
{
    int index = indexOf(mFiles,filename);
    if (index>=0)
        return mFiles[index];
    return nullptr;
}

void VisitHistoryManager::removeFile(const QString &filename)
{
    doRemove(mFiles,filename);
}

bool VisitHistoryManager::addProject(const QString &filename)
{
    PVisitRecord record = std::make_shared<VisitRecord>();
    record->filename = filename;
    return doAdd(mProjects,record,mMaxProjectCount);
}

void VisitHistoryManager::removeProject(const QString &filename)
{
    return doRemove(mProjects,filename);
}

void VisitHistoryManager::save()
{
    PVisitHistory pHistory = doLoad(mSaveFilename,mLastLoadtime);
    mergeRead(mFiles,pHistory->files);
    mergeRead(mProjects,pHistory->projects);
    QJsonObject rootObj;
    rootObj["files"] = toJson(mFiles);
    rootObj["projects"] = toJson(mProjects);
    rootObj["timestamp"] = QString("%1").arg(QDateTime::currentMSecsSinceEpoch());
    QFile file(mSaveFilename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QJsonDocument doc;
        doc.setObject(rootObj);
        file.write(doc.toJson());
    }
}

void VisitHistoryManager::load()
{
    PVisitHistory pHistory = doLoad(mSaveFilename,0);
    mFiles = pHistory->files;
    mProjects = pHistory->projects;
}

PVisitHistory VisitHistoryManager::doLoad(const QString &filename, qint64 criteriaTime)
{
    PVisitHistory pHistory=std::make_shared<VisitHistory>();
    pHistory->timestamp=0;
    QFile file(filename);
    if (!file.open(QFile::ReadOnly))
        return pHistory;
    QByteArray content = file.readAll().trimmed();
    if (content.isEmpty())
        return pHistory;
    QJsonParseError error;
    QJsonDocument doc=QJsonDocument::fromJson(content, &error);
    if (error.error!=QJsonParseError::NoError)
        return pHistory;
    bool ok;
    QJsonObject rootObj = doc.object();
    pHistory->timestamp = rootObj["timestamp"].toString().toLongLong(&ok);
    if (!ok || pHistory->timestamp < criteriaTime)
        return pHistory;
    pHistory->files = fromJson(rootObj["files"].toArray(),criteriaTime);
    pHistory->projects = fromJson(rootObj["projects"].toArray(),criteriaTime);
    mLastLoadtime = QDateTime::currentMSecsSinceEpoch();
    return pHistory;
}

QList<PVisitRecord> VisitHistoryManager::fromJson(const QJsonArray &array, qint64 criteriaTime)
{
    QList<PVisitRecord> records;
    for (int i=0;i<array.count();i++) {
        QJsonObject recordObj = array[i].toObject();
        bool ok;
        qint64 timestamp = recordObj["timestamp"].toString().toLongLong(&ok);
        if (ok && timestamp>criteriaTime) {
            PVisitRecord record = std::make_shared<VisitRecord>();
            record->timestamp = timestamp;
            record->filename = recordObj["filename"].toString();
            record->caretX = recordObj["caretX"].toInt();
            record->caretY = recordObj["caretY"].toInt();
            record->topLine = recordObj["topLine"].toInt();
            record->topSegment = recordObj["topSegment"].toInt();
            record->left = recordObj["left"].toInt();
            record->wordWrap = recordObj["wordWrap"].toBool();
            records.append(record);
        }
    }
    return records;
}

void VisitHistoryManager::mergeRead(QList<PVisitRecord> &target, const QList<PVisitRecord> &readed)
{
    QSet<QString> mergeCache;
    foreach (const PVisitRecord& r, target) {
        mergeCache.insert(r->filename);
    }
    for (int i=readed.count()-1;i>=0;i--) {
        const PVisitRecord& r=readed[i];
        if (!mergeCache.contains(r->filename)) {
            mergeCache.insert(r->filename);
            target.push_front(r);
        }
    }
}

QJsonArray VisitHistoryManager::toJson(const QList<PVisitRecord> &list)
{
    QJsonArray array;
    foreach(const PVisitRecord &record, list) {
        QJsonObject recordObj;
        recordObj["filename"]=record->filename;
        recordObj["timestamp"]=QString("%1").arg(record->timestamp);
        recordObj["caretX"]=record->caretX;
        recordObj["caretY"]=record->caretY;
        recordObj["topLine"]=record->topLine;
        recordObj["topSegment"]=record->topSegment;
        recordObj["left"]=record->left;
        recordObj["wordWrap"]=record->wordWrap;
        array.append(recordObj);
    }
    return array;
}

bool VisitHistoryManager::doAdd(QList<PVisitRecord> &list, PVisitRecord record, int maxCount)
{
    if (!QFile(record->filename).exists())
        return false;
    int index = indexOf(list,record->filename);
    if (index>=0) {
        list.removeAt(index);
    }
    if (list.size()>=maxCount) {
        list.pop_back();
    }
    record->timestamp = QDateTime::currentMSecsSinceEpoch();
    list.push_front(record);
    save();
    return true;
}

void VisitHistoryManager::doRemove(QList<PVisitRecord> &list, const QString &filename)
{
    int index = indexOf(mFiles,filename);
    if (index>=0) {
        list.removeAt(index);
    }
    save();
}

