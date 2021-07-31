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
#include <QThread>
#include <memory>
enum class DebugCommandSource {
    Console,
    Other
};

enum class AnnotationType {
  TPrePrompt, TPrompt, TPostPrompt,
  TSource,
  TDisplayBegin, TDisplayEnd,
  TDisplayExpression,
  TFrameSourceFile, TFrameSourceBegin, TFrameSourceLine, TFrameFunctionName, TFrameWhere,
  TFrameArgs,
  TFrameBegin, TFrameEnd,
  TErrorBegin, TErrorEnd,
  TArrayBegin, TArrayEnd,
  TElt, TEltRep, TEltRepEnd,
  TExit,
  TSignal, TSignalName, TSignalNameEnd, TSignalString, TSignalStringEnd,
  TValueHistoryValue, TValueHistoryBegin, TValueHistoryEnd,
  TArgBegin, TArgEnd, TArgValue, TArgNameEnd,
  TFieldBegin, TFieldEnd, TFieldValue, TFieldNameEnd,
  TInfoReg, TInfoAsm,
  TUnknown, TEOF,
  TLocal, TParam
};

struct DebugCommand{
    QString command;
    QString params;
    bool updateWatch;
    bool showInConsole;
    DebugCommandSource source;
};

using PDebugCommand = std::shared_ptr<DebugCommand>;
struct WatchVar;
using  PWatchVar = std::shared_ptr<WatchVar>;
struct WatchVar {
    QString name;
    QString text;
    int gdbIndex;
    QList<PWatchVar> children;
    WatchVar * parent; //use raw point to prevent circular-reference
};

struct Breakpoint {
    int line;
    QString filename;
    QString condition;
};

using PBreakpoint = std::shared_ptr<Breakpoint>;

struct Trace {
    QString funcname;
    QString filename;
    int line;
};

using PTrace = std::shared_ptr<Trace>;

struct Register {
    QString name;
    QString hexValue;
    QString decValue;
};

using PRegister = std::shared_ptr<Register>;

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

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    void addWatchVar(PWatchVar watchVar);
    void removeWatchVar(const QString& name);
    void removeWatchVar(int gdbIndex);
    void clear();
    const QList<PWatchVar>& watchVars();
    PWatchVar findWatchVar(const QString& name);
    PWatchVar findWatchVar(int gdbIndex);
    void notifyUpdated(PWatchVar var);
private:
    QList<PWatchVar> mWatchVars;
};


class DebugReader;
class Editor;

using PDebugReader = std::shared_ptr<DebugReader>;

class Debugger : public QObject
{
    Q_OBJECT
public:
    explicit Debugger(QObject *parent = nullptr);
    // Play/pause
    void start();
    void sendCommand(const QString& command, const QString& params,
                     bool updateWatch = true,
                     bool showInConsole = false,
                     DebugCommandSource source = DebugCommandSource::Other);

    //breakpoints
    void addBreakpoint(int line, const Editor* editor);
    void addBreakpoint(int line, const QString& filename);
    void deleteBreakpoints(const QString& filename);
    void deleteBreakpoints(const Editor* editor);
    void removeBreakpoint(int line, const Editor* editor);
    void removeBreakpoint(int line, const QString& filename);
    void removeBreakpoint(int index);
    void setBreakPointCondition(int index, const QString& condition);
    void sendAllBreakpointsToDebugger();

    //watch vars
    void addWatchVar(const QString& namein);
//    void removeWatchVar(nodein: TTreeNode); overload;
    void renameWatchVar(const QString& oldname, const QString& newname);

    void refreshWatchVars();
    void deleteWatchVars(bool deleteparent);
    void invalidateAllVars();
    void sendAllWatchvarsToDebugger();
    void invalidateWatchVar(const QString& name);
    void invalidateWatchVar(PWatchVar var);
    PWatchVar findWatchVar(const QString& name);
    void notifyWatchVarUpdated(PWatchVar var);


    void updateDebugInfo();

    bool useUTF8() const;
    void setUseUTF8(bool useUTF8);

    BacktraceModel* backtraceModel();
    BreakpointModel* breakpointModel();
    bool executing() const;

    int leftPageIndexBackup() const;
    void setLeftPageIndexBackup(int leftPageIndexBackup);

    WatchModel *watchModel() const;

public slots:
    void stop();
signals:
    void evalReady(QString value);

private:
    void sendWatchCommand(PWatchVar var);
    void sendRemoveWatchCommand(PWatchVar var);
    void sendBreakpointCommand(PBreakpoint breakpoint);
    void sendClearBreakpointCommand(int index);
    void sendClearBreakpointCommand(PBreakpoint breakpoint);

private slots:
    void syncFinishedParsing();
    void onChangeDebugConsoleLastline(const QString& text);
    void clearUpReader();
private:
    bool mExecuting;
    bool mCommandChanged;
    BreakpointModel* mBreakpointModel;
    bool mUseUTF8;
    BacktraceModel* mBacktraceModel;
    WatchModel* mWatchModel;
    DebugReader* mReader;
    int mLeftPageIndexBackup;
};

class DebugReader : public QThread
{
    Q_OBJECT
public:
    explicit DebugReader(Debugger* debugger, QObject *parent = nullptr);
    void postCommand(const QString &Command, const QString &Params,
                     bool UpdateWatch, bool ShowInConsole, DebugCommandSource  Source);
    QString debuggerPath() const;
    void setDebuggerPath(const QString &debuggerPath);
    void stopDebug();

    bool invalidateAllVars() const;
    void setInvalidateAllVars(bool invalidateAllVars);

signals:
    void parseStarted();
    void invalidateAllVars();
    void parseFinished();
    void writeToDebugFailed();
    void pauseWatchUpdate();
    void updateWatch();
    void processError(QProcess::ProcessError error);
    void changeDebugConsoleLastLine(const QString& test);
private:
    void clearCmdQueue();
    bool findAnnotation(AnnotationType annotation);
    AnnotationType getAnnotation(const QString& s);
    AnnotationType getLastAnnotation(const QByteArray& text);
    AnnotationType getNextAnnotation();
    QString getNextFilledLine();
    QString getNextLine();
    QString getNextWord();
    QString getRemainingLine();
    void handleDisassembly();
    void handleDisplay();
    void handleError();
    void handleExit();
    void handleFrames();
    void handleLocalOutput();
    void handleLocals();
    void handleParams();
    void handleRegisters();
    void handleSignal();
    void handleSource();
    void handleValueHistoryValue();
    AnnotationType peekNextAnnotation();
    void processDebugOutput();
    QString processEvalOutput();
    void processWatchOutput(PWatchVar WatchVar);
    void runNextCmd();
    void skipSpaces();
    void skipToAnnotation();
private:
    Debugger* mDebugger;
    QString mDebuggerPath;
    QRecursiveMutex mCmdQueueMutex;
    QSemaphore mStartSemaphore;
    QQueue<PDebugCommand> mCmdQueue;
    int mUpdateCount;
    bool mInvalidateAllVars;

    //fOnInvalidateAllVars: TInvalidateAllVarsEvent;
    bool mCmdRunning;
    PDebugCommand mCurrentCmd;
    QList<PRegister> mRegisters;
    QStringList mDisassembly;

    QProcess* mProcess;

    //fWatchView: TTreeView;
    int mIndex;
    int mBreakPointLine;
    QString mBreakPointFile;
    QString mOutput;
    QString mEvalValue;
    QString mSignal;
    bool mUseUTF8;

    // attempt to cut down on Synchronize calls
    bool dobacktraceready;
    bool dodisassemblerready;
    bool doregistersready;
    bool doevalready;
    bool doprocessexited;
    bool doupdatecpuwindow;
    bool doupdateexecution;
    bool doreceivedsignal;
    bool doreceivedsfwarning;

    bool mStop;
    friend class Debugger;
    // QThread interface
protected:
    void run() override;
};

#endif // DEBUGGER_H
