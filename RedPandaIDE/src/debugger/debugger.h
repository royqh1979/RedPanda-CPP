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
#include "debuggermodels.h"
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

struct DebugConfig {
    QList<PBreakpoint> breakpoints;
    QList<PWatchVar> watchVars;
    qint64 timestamp;
};

using PDebugConfig=std::shared_ptr<DebugConfig>;

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
    Debugger(const Debugger&) = delete;
    Debugger& operator= (const Debugger&) = delete;
    // Play/pause
    bool startClient(
            int compilerSetIndex,
            const QString& inferior,
            bool inferiorHasSymbols,
            bool inferiorHasBreakpoints,
            const QStringList& binDirs,
            const QString& sourceFile=QString());
    void runInferior();
    bool commandRunning() const;
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
    void addBreakpoint(int line, const QString& filename, bool forProject);
    void deleteBreakpoints(const QString& filename, bool forProject);
    void deleteBreakpoints(bool forProject);
    void deleteInvalidProjectBreakpoints(const QSet<QString> unitFiles);
    void removeBreakpoint(int line, const QString& filename, bool forProject);
    void removeBreakpoint(int index, bool forProject);
    PBreakpoint breakpointAt(int line, const QString &filename, int *index, bool forProject);
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
    bool executing();

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
    void debugFinished();
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
    void cleanUp();
    void updateRegisterNames(const QStringList& registerNames);
    void updateRegisterValues(const QHash<int,QString>& values);
    void fetchVarChildren(const QString& varName);
private:
    //bool mCommandChanged;
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
    mutable QRecursiveMutex mClientMutex;
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
    virtual bool commandRunning() const = 0;
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
    virtual void skipStandardLibraryFunctions() = 0;
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
    mutable QRecursiveMutex mCmdQueueMutex;

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
