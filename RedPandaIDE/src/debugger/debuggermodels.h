/*
 * Copyright (C) 2020-2026 Roy Qu (royqh1979@gmail.com)
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
#ifndef DEBUGGER_MODELS_H
#define DEBUGGER_MODELS_H

#include <QAbstractTableModel>
#include <QList>
#include <QList>
#include <QMap>
#include <QMutex>
#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QQueue>
#include <QSemaphore>
#include <QSet>
#include <QThread>
#include <QTimer>
#include <memory>
#include "gdbmiresultparser.h"

struct WatchVar;
using  PWatchVar = std::shared_ptr<WatchVar>;
struct WatchVar {
    QString name;
    QString expression;
    bool hasMore;
    QString value;
    QString type;
    int numChild;
    QList<PWatchVar> children;
    std::weak_ptr<WatchVar> parent; //use raw point to prevent circular-reference
    qint64 timestamp;
};

enum class BreakpointType {
    Breakpoint,
    Watchpoint,
    ReadWatchpoint,
    WriteWatchpoint
};

struct Breakpoint {
    int number; // breakpoint number
    QString type; // type of the breakpoint
    QString catch_type;
    int line;
    QString filename;
    QString condition;
    bool enabled;
    BreakpointType breakpointType;
    qint64 timestamp;
};

using PBreakpoint = std::shared_ptr<Breakpoint>;

struct Trace {
    QString funcname;
    QString filename;
    QString address;
    int line;
    int level;
};

using PTrace = std::shared_ptr<Trace>;

class RegisterModel: public QAbstractTableModel {
    Q_OBJECT
public:
    explicit RegisterModel(QObject* parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void updateNames(const QStringList& regNames);
    void updateValues(const QHash<int,QString> registerValues);
    void clear();
private:
    QHash<QString,QString> mRegisterDescriptions;
    QStringList mRegisterNames;
    QHash<int,int> mRegisterNameIndex;
    QHash<int,QString> mRegisterValues;
};

class Debugger;

class BreakpointModel: public QAbstractTableModel {
    Q_OBJECT
    // QAbstractItemModel interface
public:
    explicit BreakpointModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void addBreakpoint(PBreakpoint p, bool forProject);
    void clear(bool forProject);
    void removeBreakpoint(int index, bool forProject);
    void removeBreakpointsInFile(const QString& fileName, bool forProject);
    void renameBreakpointFilenames(const QString& oldFileName,const QString& newFileName, bool forProject);
    PBreakpoint setBreakPointCondition(int index, const QString& condition, bool forProject);
    const QList<PBreakpoint>& breakpoints(bool forProject) const {
        return forProject?mProjectBreakpoints:mBreakpoints;
    }

    PBreakpoint breakpoint(int index, bool forProject) const;

public slots:
    void updateBreakpointNumber(const QString& filename, int line, int number);
    void invalidateAllBreakpointNumbers(); // call this when gdb is stopped
    void onFileDeleteLines(const QString& filename, int startLine, int count, bool forProject);
    void onFileInsertLines(const QString& filename, int startLine, int count, bool forProject);
    void onFileLineMoved(const QString& filename, int fromLine, int toLine, bool forProject);
private:
    bool isForProject() const;
    void setIsForProject(bool newIsForProject);
    QList<PBreakpoint> loadJson(const QJsonArray& jsonArray, qint64 criteriaTime);
    QJsonArray toJson(const QString& projectFolder);
    void setBreakpoints(const QList<PBreakpoint>& list, bool forProject);

private:
    QList<PBreakpoint> mBreakpoints;
    QList<PBreakpoint> mProjectBreakpoints;
    bool mIsForProject;

    friend class Debugger;
};

class BacktraceModel : public QAbstractTableModel {
    Q_OBJECT
    // QAbstractItemModel interface
public:
    explicit BacktraceModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void setTraces(QList<PTrace> traces);
    void clear();
    const QList<PTrace>& backtraces() const;
    PTrace backtrace(int index) const;
private:
    QList<PTrace> mList;
};

class WatchModel: public QAbstractItemModel {
    Q_OBJECT
public:
    explicit WatchModel(QObject *parent = nullptr);
    QVariant data(const QModelIndex &index, int role) const override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void fetchMore(const QModelIndex &parent) override;
    bool canFetchMore(const QModelIndex &parent) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    bool hasChildren(const QModelIndex &parent) const override;
    QModelIndex index(PWatchVar var) const;
    QModelIndex index(WatchVar* pVar) const;

    void removeWatchVar(const QString& expression);
    void removeWatchVar(const QModelIndex& index);
    void clear();
    void clear(bool forProject);
    PWatchVar findWatchVar(const QModelIndex& index);
    PWatchVar findWatchVar(const QString& expr);
    void resetAllVarInfos();
    void clearAllVarInfos();
    void beginUpdate();
    void endUpdate();
    void notifyUpdated(PWatchVar var);
signals:
    void setWatchVarValue(const QString& name, const QString& value);
public  slots:
    void updateVarInfo(const QString& expression,
                    const QString& name,
                    int numChild,
                    const QString& value,
                    const QString& type,
                    bool hasMore);
    void prepareVarChildren(const QString& parentName, int numChild, bool hasMore);
    void addVarChild(const QString& parentName, const QString& name,
                     const QString& exp, int numChild,
                     const QString& value, const QString& type,
                     bool hasMore);
    void updateVarValue(const QString& name, const QString& val,
                         const QString& inScope, bool typeChanged,
                         const QString& newType, int newNumChildren,
                         bool hasMore);
    void updateAllHasMoreVars();
signals:
    void fetchChildren(const QString& name);
private:
    bool isForProject() const;
    void setIsForProject(bool newIsForProject);
    const QList<PWatchVar> &watchVars(bool forProject) const;
    QJsonArray toJson(bool forProject);
    QList<PWatchVar> loadJson(const QJsonArray &jsonArray, qint64 criteriaTimestamp);
    const QList<PWatchVar> &watchVars() const;
    void addWatchVar(PWatchVar watchVar, bool forProject);
    void setWatchVars(const QList<PWatchVar> list, bool forProject);

private:
    QList<PWatchVar> mWatchVars;
    QList<PWatchVar> mProjectWatchVars;

    QHash<QString,PWatchVar> mVarIndex; //var index is only valid for the current debugging session

    int mUpdateCount;
    bool mIsForProject;

    // QAbstractItemModel interface
public:
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

    friend class Debugger;
};

struct MemoryLine {
    uintptr_t startAddress;
    QList<unsigned char> datas;
    QSet<int> changedDatas;
};

using PMemoryLine = std::shared_ptr<MemoryLine>;

class MemoryModel: public QAbstractTableModel{
    Q_OBJECT
public:
    explicit MemoryModel(int dataPerLine,QObject* parent=nullptr);

    void updateMemory(const QStringList& value);
    qulonglong startAddress() const;
    void reset();
    // QAbstractItemModel interface
signals:
    void setMemoryData(qlonglong address, unsigned char data);
public:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;

private:
    int mDataPerLine;
    QList<PMemoryLine> mLines;
    qulonglong mStartAddress;
};

#endif // DEBUGGER_MODELS_H
