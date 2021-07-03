#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <QList>
#include <QList>
#include <QMutex>
#include <QObject>
#include <QQueue>
#include <QQueue>
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
    int value;
};

using PRegister = std::shared_ptr<Register>;

class DebugReader;

class Debugger : public QObject
{
    Q_OBJECT
public:
    explicit Debugger(QObject *parent = nullptr);

signals:
private:
    bool mExecuting;
    bool mCommandChanged;
    QList<PBreakpoint> mBreakpointList;
    bool mUseUTF8;
    QString getBreakpointFile();
};

class DebugReader : public QObject
{
    Q_OBJECT
public:
    explicit DebugReader(QObject *parent = nullptr);

private:
    void clearCmdQueue();
    bool findAnnotation(AnnotationType annotation);
    AnnotationType getAnnotation(const QString& s);
    AnnotationType getLastAnnotation(const QString& text,int curpos, int len;
    AnnotationType getNextAnnotation();
private:
    QMutex mMutex;
    QQueue<PDebugCommand> mCmdQueue;
    int mUpdateCount;
    bool mInvalidateAllVars;

    //fOnInvalidateAllVars: TInvalidateAllVarsEvent;
    bool mCmdRunning;
    PDebugCommand mCurrentCmd;
    QList<PRegister> mRegisters;
    QStringList mDisassembly;
    QList<PTrace> mBacktrace;
    QList<PBreakpoint> mBreakpoints;
    //fWatchVarList: TList; // contains all parents
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
    bool dorescanwatches;
    bool doevalready;
    bool doprocessexited;
    bool doupdatecpuwindow;
    bool doupdateexecution;
    bool doreceivedsignal;
    bool doreceivedsfwarning;
};

#endif // DEBUGGER_H
