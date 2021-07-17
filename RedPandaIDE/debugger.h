#ifndef DEBUGGER_H
#define DEBUGGER_H

#include <QAbstractTableModel>
#include <QList>
#include <QList>
#include <QMap>
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

struct WatchVar {
    QString name;
    int gdbIndex;
};

using PWatchVar = std::shared_ptr<WatchVar>;

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
    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void addBreakpoint(PBreakpoint p);
    void clear();
    void removeBreakpoint(int row);
private:
    QList<PBreakpoint> mList;
};

class BacktraceModel : public QAbstractTableModel {
    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    int columnCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role) const override;
    void addTrace(PTrace p);
    void clear();
    void removeTrace(int row);
private:
    QList<PTrace> mList;
};


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
    AnnotationType getLastAnnotation(const QString& text,int curpos, int len);
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
    BacktraceModel mBacktraceModel;

    QMap<QString,PWatchVar> mWatchVarList; // contains all parents
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
