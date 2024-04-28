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

enum class DebuggerType {
    GDB,
    LLDB_MI,
    DAP
};

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

struct DebugConfig {
    QList<PBreakpoint> breakpoints;
    QList<PWatchVar> watchVars;
    qint64 timestamp;
};


using PDebugConfig=std::shared_ptr<DebugConfig>;

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


class DebuggerClient;
class DebugTarget;
class Editor;

using PDebugReader = std::shared_ptr<DebuggerClient>;

class Debugger : public QObject
{
    Q_OBJECT
public:
    explicit Debugger(QObject *parent = nullptr);
    ~Debugger();
    // Play/pause
    bool startClient(
            int compilerSetIndex,
            const QString& inferior,
            bool inferiorHasSymbols,
            bool inferiorHasBreakpoints,
            const QStringList& binDirs,
            const QString& sourceFile=QString());
    void runInferior();
    bool commandRunning();
    bool inferiorRunning();
    void interrupt();
    void stepOver();
    void stepInto();
    void stepOut();
    void runTo(const QString& filename, int line);
    void resume();
    void stepOverInstruction();
    void stepIntoInstruction();

    void runClientCommand(const QString &command, const QString &params, DebugCommandSource source);

    bool isForProject() const;
    void setIsForProject(bool newIsForProject);
    void clearForProject();

    //breakpoints
    void addBreakpoint(int line, const Editor* editor);
    void addBreakpoint(int line, const QString& filename, bool forProject);
    void deleteBreakpoints(const QString& filename, bool forProject);
    void deleteBreakpoints(const Editor* editor);
    void deleteBreakpoints(bool forProject);
    void deleteInvalidProjectBreakpoints(const QSet<QString> unitFiles);
    void removeBreakpoint(int line, const Editor* editor);
    void removeBreakpoint(int line, const QString& filename, bool forProject);
    void removeBreakpoint(int index, bool forProject);
    PBreakpoint breakpointAt(int line, const QString &filename, int *index, bool forProject);
    PBreakpoint breakpointAt(int line, const Editor *editor, int *index);
    void setBreakPointCondition(int index, const QString& condition, bool forProject);
    void sendAllBreakpointsToDebugger();

    void saveForNonproject(const QString &filename);
    void saveForProject(const QString &filename, const QString &projectFolder);

    void loadForNonproject(const QString &filename);
    void loadForProject(const QString& filename, const QString& projectFolder);

    void addWatchpoint(const QString& expression);
    //watch vars
    void addWatchVar(const QString& expression);
    void modifyWatchVarExpression(const QString& oldExpr, const QString& newExpr);

    void removeWatchVars(bool deleteparent);
    void removeWatchVar(const QModelIndex& index);
    void sendAllWatchVarsToDebugger();
    PWatchVar findWatchVar(const QString& expression);
    PWatchVar watchVarAt(const QModelIndex& index);
    void refreshWatchVars();

    void readMemory(const QString& startAddress, int rows, int cols);
    void evalExpression(const QString& expression);
    void selectFrame(PTrace trace);
    void refreshFrame();
    void refreshStackVariables();
    void refreshRegisters();
    void disassembleCurrentFrame(bool blendMode);
    void setDisassemblyLanguage(bool isIntel);
    void includeOrSkipDirsInSymbolSearch(const QStringList &dirs, bool skip);

//    void notifyWatchVarUpdated(PWatchVar var);

    std::shared_ptr<BacktraceModel> backtraceModel();
    std::shared_ptr<BreakpointModel> breakpointModel();
    bool executing() const;

    int leftPageIndexBackup() const;
    void setLeftPageIndexBackup(int leftPageIndexBackup);

    std::shared_ptr<WatchModel> watchModel() const;

    std::shared_ptr<RegisterModel> registerModel() const;

    std::shared_ptr<MemoryModel> memoryModel() const;

    bool forceUTF8() const;
    void setForceUTF8(bool newForceUTF8);

    DebuggerType debuggerType() const;
    void setDebuggerType(DebuggerType newDebuggerType);

    bool debugInfosUsingUTF8() const;
    void setDebugInfosUsingUTF8(bool newDebugInfosUsingUTF8);

    bool useDebugServer() const;
    void setUseDebugServer(bool newUseDebugServer);
    bool supportDisassemlyBlendMode();
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
    void sendClearBreakpointCommand(int index, bool forProject);
    void sendClearBreakpointCommand(PBreakpoint breakpoint);
    void save(const QString& filename, const QString& projectFolder);
    PDebugConfig load(const QString& filename, bool forProject);
    void addWatchVar(const PWatchVar &watchVar, bool forProject);

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
    void fetchVarChildren(const QString& varName);
private:
    bool mExecuting;
    bool mCommandChanged;
    std::shared_ptr<BreakpointModel> mBreakpointModel;
    std::shared_ptr<BacktraceModel> mBacktraceModel;
    std::shared_ptr<WatchModel> mWatchModel;
    std::shared_ptr<RegisterModel> mRegisterModel;
    std::shared_ptr<MemoryModel> mMemoryModel;
    DebuggerClient *mClient;
    DebugTarget *mTarget;
    bool mForceUTF8;
    bool mDebugInfosUsingUTF8;
    bool mUseDebugServer;
    DebuggerType mDebuggerType;
    int mLeftPageIndexBackup;
    qint64 mLastLoadtime;
    qint64 mProjectLastLoadtime;
    QString mCurrentSourceFile;
    bool mInferiorHasBreakpoints;
};

class DebugTarget: public QThread {
    Q_OBJECT
public:
    explicit DebugTarget(const QString& inferior,
                         const QString& GDBServer,
                         int port,
                         const QStringList& arguments,
                         QObject *parent = nullptr);
    void setInputFile(const QString& inputFile);
    void stopDebug();
    void waitStart();
    const QStringList &binDirs() const;
    void addBinDirs(const QStringList &binDirs);
    void addBinDir(const QString &binDir);
signals:
    void processFailed(QProcess::ProcessError error);
private:
    QString mInferior;
    QStringList mArguments;
    QString mGDBServer;
    int mPort;
    bool mStop;
    std::shared_ptr<QProcess> mProcess;
    QSemaphore mStartSemaphore;
    bool mErrorOccured;
    QString mInputFile;
    QStringList mBinDirs;

    // QThread interface
protected:
    void run() override;
};

class DebuggerClient : public QThread
{
    Q_OBJECT
public:
    explicit DebuggerClient(Debugger* debugger, QObject *parent = nullptr);
    virtual void stopDebug() = 0;
    virtual bool commandRunning() = 0;
    QString debuggerPath() const;
    void setDebuggerPath(const QString &debuggerPath);
    void waitStart();

    bool processExited() const;

    bool signalReceived() const;

    const QStringList &consoleOutput() const;

    bool updateCPUInfo() const;

    bool receivedSFWarning() const;

    const QStringList &fullOutput() const;

    bool inferiorRunning() const;

    const QString &signalName() const;

    const QString &signalMeaning() const;

    const QStringList &binDirs() const;
    void addBinDirs(const QStringList &binDirs);
    void addBinDir(const QString &binDir);

    Debugger* debugger() { return mDebugger; }

    virtual DebuggerType clientType() = 0;

    //requests
    virtual void initialize(const QString& inferior, bool hasSymbols) = 0;
    virtual void runInferior(bool hasBreakpoints) = 0;

    virtual void stepOver() = 0;
    virtual void stepInto() = 0;
    virtual void stepOut() = 0;
    virtual void runTo(const QString& filename, int line) = 0;
    virtual void resume() = 0;
    virtual void stepOverInstruction() = 0;
    virtual void stepIntoInstruction() = 0;
    virtual void interrupt() = 0;

    virtual void refreshStackVariables() = 0;

    virtual void readMemory(const QString& startAddress, int rows, int cols) = 0;
    virtual void writeMemory(qulonglong address, unsigned char data) = 0;

    virtual void addBreakpoint(PBreakpoint breakpoint) = 0;
    virtual void removeBreakpoint(PBreakpoint breakpoint) = 0;
    virtual void addWatchpoint(const QString& watchExp) = 0;
    virtual void setBreakpointCondition(PBreakpoint breakpoint) = 0;

    virtual void addWatch(const QString& expression) = 0;
    virtual void removeWatch(PWatchVar watchVar) = 0;
    virtual void writeWatchVar(const QString& varName, const QString& value) = 0;
    virtual void refreshWatch(PWatchVar var) = 0;
    virtual void refreshWatch() = 0;
    virtual void fetchWatchVarChildren(const QString& varName) = 0;

    virtual void evalExpression(const QString& expression) = 0;

    virtual void selectFrame(PTrace trace) = 0;
    virtual void refreshFrame() = 0;
    virtual void refreshRegisters() = 0;
    virtual void disassembleCurrentFrame(bool blendMode) = 0;
    virtual void setDisassemblyLanguage(bool isIntel) = 0;

    virtual void skipDirectoriesInSymbolSearch(const QStringList& lst) = 0;
    virtual void addSymbolSearchDirectories(const QStringList& lst) = 0;
signals:
    void parseStarted();
    void invalidateAllVars();
    void parseFinished();
    void writeToDebugFailed();
    void processFailed(QProcess::ProcessError error);
    void changeDebugConsoleLastLine(const QString& text);
    void cmdStarted();
    void cmdFinished();

    void errorNoSymbolTable();
    void breakpointInfoGetted(const QString& filename, int line, int number);
    void inferiorContinued();
    void watchpointHitted(const QString& var, const QString& oldVal, const QString& newVal);
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
    void varsValueUpdated();
protected:
    QRecursiveMutex mCmdQueueMutex;

    bool mCmdRunning;

    bool mInferiorRunning;
    bool mProcessExited;

    QStringList mConsoleOutput;
    QStringList mFullOutput;
    QSemaphore mStartSemaphore;
    bool mSignalReceived;
    bool mUpdateCPUInfo;

    QString mSignalName;
    QString mSignalMeaning;
    bool mReceivedSFWarning;
private:
    Debugger *mDebugger;
    QString mDebuggerPath;

    QStringList mBinDirs;

};

#endif // DEBUGGER_H
