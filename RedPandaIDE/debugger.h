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
#ifndef DEBUGGER_H
#define DEBUGGER_H

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

enum class DebugCommandSource {
    Console,
    HeartBeat,
    Other
};

struct DebugCommand{
    QString command;
    QString params;
    DebugCommandSource source;
};

using PDebugCommand = std::shared_ptr<DebugCommand>;
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
    WatchVar * parent; //use raw point to prevent circular-reference
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
    QStringList mRegisterNames;
    QHash<int,QString> mRegisterValues;
};

class BreakpointModel: public QAbstractTableModel {
    Q_OBJECT
    // QAbstractItemModel interface
public:
    explicit BreakpointModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void addBreakpoint(PBreakpoint p);
    void clear();
    void removeBreakpoint(int index);
    PBreakpoint setBreakPointCondition(int index, const QString& condition);
    const QList<PBreakpoint>& breakpoints() const;
    PBreakpoint breakpoint(int index) const;
    void save(const QString& filename);
    void load(const QString& filename);
public slots:
    void updateBreakpointNumber(const QString& filename, int line, int number);
    void invalidateAllBreakpointNumbers(); // call this when gdb is stopped
    void onFileDeleteLines(const QString& filename, int startLine, int count);
    void onFileInsertLines(const QString& filename, int startLine, int count);
private:
    QList<PBreakpoint> mList;
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
    void addTrace(PTrace p);
    void clear();
    void removeTrace(int index);
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
    void addWatchVar(PWatchVar watchVar);
    void removeWatchVar(const QString& expression);
    void removeWatchVar(const QModelIndex& index);
    void clear();
    const QList<PWatchVar>& watchVars();
    PWatchVar findWatchVar(const QModelIndex& index);
    PWatchVar findWatchVar(const QString& expr);
    void resetAllVarInfos();
    void clearAllVarInfos();
    void beginUpdate();
    void endUpdate();
    void notifyUpdated(PWatchVar var);
    void save(const QString& filename);
    void load(const QString& filename);
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
signals:
    void fetchChildren(const QString& name);
private:
    QModelIndex index(PWatchVar var) const;
    QModelIndex index(WatchVar* pVar) const;
private:
    QList<PWatchVar> mWatchVars;
    QHash<QString,PWatchVar> mVarIndex;
    int mUpdateCount;

    // QAbstractItemModel interface
public:
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
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


class DebugReader;
class DebugTarget;
class Editor;

using PDebugReader = std::shared_ptr<DebugReader>;

class Debugger : public QObject
{
    Q_OBJECT
public:
    explicit Debugger(QObject *parent = nullptr);
    // Play/pause
    bool start(const QString& inferior);
    void sendCommand(const QString& command, const QString& params,
                     DebugCommandSource source = DebugCommandSource::Other);
    bool commandRunning();
    bool inferiorRunning();
    void interrupt();

    //breakpoints
    void addBreakpoint(int line, const Editor* editor);
    void addBreakpoint(int line, const QString& filename);
    void deleteBreakpoints(const QString& filename);
    void deleteBreakpoints(const Editor* editor);
    void deleteBreakpoints();
    void removeBreakpoint(int line, const Editor* editor);
    void removeBreakpoint(int line, const QString& filename);
    void removeBreakpoint(int index);
    PBreakpoint breakpointAt(int line, const QString& filename, int &index);
    PBreakpoint breakpointAt(int line, const Editor* editor, int &index);
    void setBreakPointCondition(int index, const QString& condition);
    void sendAllBreakpointsToDebugger();

    //watch vars
    void addWatchVar(const QString& expression);
    void modifyWatchVarExpression(const QString& oldExpr, const QString& newExpr);

    void removeWatchVars(bool deleteparent);
    void removeWatchVar(const QModelIndex& index);
    void sendAllWatchVarsToDebugger();
    PWatchVar findWatchVar(const QString& expression);
//    void notifyWatchVarUpdated(PWatchVar var);

    BacktraceModel* backtraceModel();
    BreakpointModel* breakpointModel();
    bool executing() const;

    int leftPageIndexBackup() const;
    void setLeftPageIndexBackup(int leftPageIndexBackup);

    WatchModel *watchModel() const;

    RegisterModel *registerModel() const;

    MemoryModel *memoryModel() const;

signals:
    void evalValueReady(const QString& s);
    void memoryExamineReady(const QStringList& s);
    void localsReady(const QStringList& s);
public slots:
    void stop();
    void refreshAll();

private:
    void sendWatchCommand(PWatchVar var);
    void sendRemoveWatchCommand(PWatchVar var);
    void sendBreakpointCommand(PBreakpoint breakpoint);
    void sendClearBreakpointCommand(int index);
    void sendClearBreakpointCommand(PBreakpoint breakpoint);

private slots:
    void syncFinishedParsing();
    void setMemoryData(qulonglong address, unsigned char data);
    void setWatchVarValue(const QString& name, const QString& value);
    void updateMemory(const QStringList& value);
    void updateEval(const QString& value);
    void updateDisassembly(const QString& file, const QString& func,const QStringList& value);
    void onChangeDebugConsoleLastline(const QString& text);
    void cleanUpReader();
    void updateRegisterNames(const QStringList& registerNames);
    void updateRegisterValues(const QHash<int,QString>& values);
    void refreshWatchVars();
    void fetchVarChildren(const QString& varName);
private:
    bool mExecuting;
    bool mCommandChanged;
    BreakpointModel *mBreakpointModel;
    BacktraceModel *mBacktraceModel;
    WatchModel *mWatchModel;
    RegisterModel *mRegisterModel;
    MemoryModel *mMemoryModel;
    DebugReader *mReader;
    DebugTarget *mTarget;
    int mLeftPageIndexBackup;
};

class DebugTarget: public QThread {
    Q_OBJECT
public:
    explicit DebugTarget(const QString& inferior,
                         const QString& GDBServer,
                         int port,
                         QObject *parent = nullptr);
    void setInputFile(const QString& inputFile);
    void stopDebug();
    void waitStart();
signals:
    void processError(QProcess::ProcessError error);
private:
    QString mInferior;
    QString mGDBServer;
    int mPort;
    bool mStop;
    std::shared_ptr<QProcess> mProcess;
    QSemaphore mStartSemaphore;
    bool mErrorOccured;
    QString mInputFile;

    // QThread interface
protected:
    void run() override;
};

class DebugReader : public QThread
{
    Q_OBJECT
public:
    explicit DebugReader(Debugger* debugger, QObject *parent = nullptr);
    void postCommand(const QString &Command, const QString &Params, DebugCommandSource  Source);
    void registerInferiorStoppedCommand(const QString &Command, const QString &Params);
    QString debuggerPath() const;
    void setDebuggerPath(const QString &debuggerPath);
    void stopDebug();

    bool commandRunning();
    void waitStart();

    bool inferiorPaused() const;

    bool processExited() const;

    bool signalReceived() const;

    const QStringList &consoleOutput() const;

    int breakPointLine() const;

    const QString &breakPointFile() const;

    const PDebugCommand &currentCmd() const;

    bool updateCPUInfo() const;

    bool updateLocals() const;

    const QStringList &localsValue() const;

    bool evalReady() const;

    const QString &evalValue() const;

    bool updateMemory() const;

    const QStringList &memoryValue() const;

    bool receivedSFWarning() const;

    const QStringList &fullOutput() const;

    bool inferiorRunning() const;

    const QString &signalName() const;

    const QString &signalMeaning() const;

signals:
    void parseStarted();
    void invalidateAllVars();
    void parseFinished();
    void writeToDebugFailed();
    void processError(QProcess::ProcessError error);
    void changeDebugConsoleLastLine(const QString& text);
    void cmdStarted();
    void cmdFinished();

    void breakpointInfoGetted(const QString& filename, int line, int number);
    void inferiorContinued();
    void inferiorStopped(const QString& filename, int line, bool setFocus);
    void localsUpdated(const QStringList& localsValue);
    void evalUpdated(const QString& value);
    void memoryUpdated(const QStringList& memoryValues);
    void disassemblyUpdate(const QString& filename, const QString& funcName, const QStringList& result);
    void registerNamesUpdated(const QStringList& registerNames);
    void registerValuesUpdated(const QHash<int,QString>& values);
    void varCreated(const QString& expression,
                    const QString& name,
                    int numChild,
                    const QString& value,
                    const QString& type,
                    bool hasMore);
    void prepareVarChildren(const QString& parentName,int numChild, bool hasMore);
    void addVarChild(const QString& parentName, const QString& name,
                     const QString& exp, int numChild,
                     const QString& value, const QString& type,
                     bool hasMore);
    void varValueUpdated(const QString& name, const QString& val,
                         const QString& inScope, bool typeChanged,
                         const QString& newType, int newNumChildren,
                         bool hasMore);
private:
    void clearCmdQueue();

    void runNextCmd();
    QStringList tokenize(const QString& s);

    bool outputTerminated(const QByteArray& text);
    void handleBreakpoint(const GDBMIResultParser::ParseObject& breakpoint);
    void handleStack(const QList<GDBMIResultParser::ParseValue> & stack);
    void handleLocalVariables(const QList<GDBMIResultParser::ParseValue> & variables);
    void handleEvaluation(const QString& value);
    void handleMemory(const QList<GDBMIResultParser::ParseValue> & rows);
    void handleRegisterNames(const QList<GDBMIResultParser::ParseValue> & names);
    void handleRegisterValue(const QList<GDBMIResultParser::ParseValue> & values);
    void handleCreateVar(const GDBMIResultParser::ParseObject& multiVars);
    void handleListVarChildren(const GDBMIResultParser::ParseObject& multiVars);
    void handleUpdateVarValue(const QList<GDBMIResultParser::ParseValue> &changes);
    void processConsoleOutput(const QByteArray& line);
    void processResult(const QByteArray& result);
    void processExecAsyncRecord(const QByteArray& line);
    void processError(const QByteArray& errorLine);
    void processResultRecord(const QByteArray& line);
    void processDebugOutput(const QByteArray& debugOutput);
    void runInferiorStoppedHook();
    QByteArray removeToken(const QByteArray& line);
private slots:
    void asyncUpdate();
private:
    Debugger *mDebugger;
    QString mDebuggerPath;
    QMutex mCmdQueueMutex;
    QSemaphore mStartSemaphore;
    QQueue<PDebugCommand> mCmdQueue;
    bool mErrorOccured;
    bool mAsyncUpdated;
    //fOnInvalidateAllVars: TInvalidateAllVarsEvent;
    bool mCmdRunning;
    PDebugCommand mCurrentCmd;
    std::shared_ptr<QProcess> mProcess;

    //fWatchView: TTreeView;

    QString mSignalName;
    QString mSignalMeaning;

    //
    QList<PDebugCommand> mInferiorStoppedHookCommands;
    bool mInferiorRunning;
    bool mProcessExited;

    bool mSignalReceived;
    bool mUpdateCPUInfo;
    bool mReceivedSFWarning;

    int mCurrentLine;
    int mCurrentAddress;
    QString mCurrentFunc;
    QString mCurrentFile;
    QStringList mConsoleOutput;
    QStringList mFullOutput;
    bool mStop;
    // QThread interface
protected:
    void run() override;
};

#endif // DEBUGGER_H
