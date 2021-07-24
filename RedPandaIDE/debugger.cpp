#include "debugger.h"
#include "utils.h"
#include "mainwindow.h"

Debugger::Debugger(QObject *parent) : QObject(parent)
{

}

bool Debugger::useUTF8() const
{
    return mUseUTF8;
}

void Debugger::setUseUTF8(bool useUTF8)
{
    mUseUTF8 = useUTF8;
}

const BacktraceModel* Debugger::getBacktraceModel() const
{
    return mBacktraceModel;
}

DebugReader::DebugReader(QObject *parent) : QObject(parent)
{

}

void DebugReader::postCommand(const QString &Command, const QString &Params, bool UpdateWatch, bool ShowInConsole, DebugCommandSource Source)
{
    QMutexLocker locker(&mCmdQueueMutex);
    if (mCmdQueue.isEmpty() && UpdateWatch) {
        emit pauseWatchUpdate();
        mUpdateCount++;
    }
    PDebugCommand pCmd = std::make_shared<DebugCommand>();
    pCmd->command = Command;
    pCmd->params = Params;
    pCmd->updateWatch = UpdateWatch;
    pCmd->showInConsole = ShowInConsole;
    pCmd->source = Source;
    mCmdQueue.enqueue(pCmd);
    if (!mCmdRunning)
        runNextCmd();
}

void DebugReader::clearCmdQueue()
{
    QMutexLocker locker(&mCmdQueueMutex);
    mCmdQueue.clear();

    if (mUpdateCount>0) {
        emit updateWatch();
        mUpdateCount=0;
    }
}

bool DebugReader::findAnnotation(AnnotationType annotation)
{
    AnnotationType NextAnnotation;
    do {
        NextAnnotation = getNextAnnotation();
        if (NextAnnotation == AnnotationType::TEOF)
            return false;
    } while (NextAnnotation != annotation);

    return true;
}

AnnotationType DebugReader::getAnnotation(const QString &s)
{
    if (s == "pre-prompt") {
        return AnnotationType::TPrePrompt;
    } else if (s == "prompt") {
        return AnnotationType::TPrompt;
    } else if (s == "post-prompt") {
      AnnotationType result = AnnotationType::TPostPrompt;

      int IndexBackup = mIndex;
      QString t = getNextFilledLine();
      int mIndex = IndexBackup;

      //hack to catch local
      if ((mCurrentCmd) && (mCurrentCmd->command == "info locals")) {
          result = AnnotationType::TLocal;
      } else if ((mCurrentCmd) && (mCurrentCmd->command == "info args")) {
          //hack to catch params
          result = AnnotationType::TParam;
      } else if (t.startsWith("rax ") || t.startsWith("eax ")) {
          // Hack fix to catch register dump
          result = AnnotationType::TInfoReg;
      } else {
          // Another hack to catch assembler
          if (t.startsWith("Dump of assembler code for function "))
          result = AnnotationType::TInfoAsm;
      }
      return result;
    } else if (s == "error-begin") {
        return AnnotationType::TErrorBegin;
    } else if (s == "error-end") {
      return AnnotationType::TErrorEnd;
    } else if (s == "display-begin") {
      return AnnotationType::TDisplayBegin;
    } else if (s == "display-expression") {
      return AnnotationType::TDisplayExpression;
    } else if (s == "display-end") {
      return AnnotationType::TDisplayEnd;
    } else if (s == "frame-source-begin") {
      return AnnotationType::TFrameSourceBegin;
    } else if (s == "frame-source-file") {
      return AnnotationType::TFrameSourceFile;
    } else if (s == "frame-source-line") {
      return AnnotationType::TFrameSourceLine;
    } else if (s == "frame-function-name") {
      return AnnotationType::TFrameFunctionName;
    } else if (s == "frame-args") {
      return AnnotationType::TFrameArgs;
    } else if (s == "frame-begin") {
      return AnnotationType::TFrameBegin;
    } else if (s == "frame-end") {
      return AnnotationType::TFrameEnd;
    } else if (s == "frame-where") {
      return AnnotationType::TFrameWhere;
    } else if (s == "source") {
      return AnnotationType::TSource;
    } else if (s == "exited") {
      return AnnotationType::TExit;
    } else if (s == "arg-begin") {
      return AnnotationType::TArgBegin;
    } else if (s == "arg-name-end") {
      return AnnotationType::TArgNameEnd;
    } else if (s == "arg-value") {
      return AnnotationType::TArgValue;
    } else if (s == "arg-end") {
      return AnnotationType::TArgEnd;
    } else if (s == "array-section-begin") {
      return AnnotationType::TArrayBegin;
    } else if (s == "array-section-end") {
      return AnnotationType::TArrayEnd;
    } else if (s == "elt") {
      return AnnotationType::TElt;
    } else if (s == "elt-rep") {
      return AnnotationType::TEltRep;
    } else if (s == "elt-rep-end") {
      return AnnotationType::TEltRepEnd;
    } else if (s == "field-begin") {
      return AnnotationType::TFieldBegin;
    } else if (s == "field-name-end") {
      return AnnotationType::TFieldNameEnd;
    } else if (s == "field-value") {
      return AnnotationType::TFieldValue;
    } else if (s == "field-end") {
      return AnnotationType::TFieldEnd;
    } else if (s == "value-history-value") {
      return AnnotationType::TValueHistoryValue;
    } else if (s == "value-history-begin") {
      return AnnotationType::TValueHistoryBegin;
    } else if (s == "value-history-end") {
      return AnnotationType::TValueHistoryEnd;
    } else if (s == "signal") {
      return AnnotationType::TSignal;
    } else if (s == "signal-name") {
      return AnnotationType::TSignalName;
    } else if (s == "signal-name-end") {
      return AnnotationType::TSignalNameEnd;
    } else if (s == "signal-string") {
      return AnnotationType::TSignalString;
    } else if (s == "signal-string-end") {
      return AnnotationType::TSignalStringEnd;
    } else if (mIndex >= mOutput.length()) {
      return AnnotationType::TEOF;
    } else {
      return AnnotationType::TUnknown;;
    }
}

AnnotationType DebugReader::getLastAnnotation(const QByteArray &text)
{
    int curpos = text.length()-1;
    // Walk back until end of #26's
    while ((curpos >= 0) && (text[curpos] != 26))
        curpos--;

    curpos++;

    // Tiny rewrite of GetNextWord for special purposes
    QString s = "";
    while ((curpos < text.length()) && (text[curpos]>32)) {
        s = s + text[curpos];
        curpos++;
    }

    return getAnnotation(s);
}

AnnotationType DebugReader::getNextAnnotation()
{
    // Skip until end of #26's, i.e. GDB formatted output
    skipToAnnotation();

    // Get part this line, after #26#26
    return getAnnotation(getNextWord());
}

QString DebugReader::getNextFilledLine()
{
    // Walk up to an enter sequence
    while (mIndex<mOutput.length() && mOutput[mIndex]!=13 && mOutput[mIndex]!=10 && mOutput[mIndex]!=0)
        mIndex++;
    // Skip enter sequences (CRLF, CR, LF, etc.)
    while (mIndex<mOutput.length() && mOutput[mIndex]==13 && mOutput[mIndex]==10 && mOutput[mIndex]==0)
        mIndex++;
    // Return next line
    return getRemainingLine();
}

QString DebugReader::getNextLine()
{
    // Walk up to an enter sequence
    while (mIndex<mOutput.length() && mOutput[mIndex]!=13 && mOutput[mIndex]!=10 && mOutput[mIndex]!=0)
        mIndex++;

    // End of output. Exit
    if (mIndex>=mOutput.length())
        return "";
    // Skip ONE enter sequence (CRLF, CR, LF, etc.)
    if ((mOutput[mIndex] == 13) && (mOutput[mIndex] == 10)) // DOS
        mIndex+=2;
    else if (mOutput[mIndex] == 13)  // UNIX
        mIndex++;
    else if (mOutput[mIndex] == 10) // MAC
        mIndex++;
    // Return next line
    return getRemainingLine();
}

QString DebugReader::getNextWord()
{
    QString Result;

    // Called when at a space? Skip over
    skipSpaces();

    // Skip until a space
    while (mIndex<mOutput.length() && mOutput[mIndex]>32) {
        Result += mOutput[mIndex];
        mIndex++;
    }
    return Result;
}

QString DebugReader::getRemainingLine()
{
    QString Result;

    // Return part of line still ahead of us
    while (mIndex<mOutput.length() && mOutput[mIndex]!=13 && mOutput[mIndex]!=10 && mOutput[mIndex]!=0) {
        Result += mOutput[mIndex];
        mIndex++;
    }
    return Result;
}

void DebugReader::handleDisassembly()
{
    if (mDisassembly.isEmpty())
        return;

    // Get info message
    QString s = getNextLine();

    // the full function name will be saved at index 0
    mDisassembly.append(s.mid(36));

    s = getNextLine();

    // Add lines of disassembly
    while (!s.isEmpty() && (s != "End of assembler dump")) {
        mDisassembly.append(s);
        s = getNextLine();
    }

    dodisassemblerready = true;
}

void DebugReader::handleDisplay()
{
    QString s = getNextLine(); // watch index

    if (!findAnnotation(AnnotationType::TDisplayExpression))
        return;
    QString watchName = getNextLine(); // watch name

    // Find parent we're talking about
    auto result = mWatchVarList.find(watchName);
    if (result != mWatchVarList.end()) {
        PWatchVar watchVar = result.value();
        // Advance up to the value
        if (!findAnnotation(AnnotationType::TDisplayExpression))
            return;;
        // Refresh GDB index so we can undisplay this by index
        watchVar->gdbIndex = s.toInt();
        processWatchOutput(watchVar);
    }
}

void DebugReader::handleError()
{
    QString s = getNextLine(); // error text
    if (s.startsWith("Cannot find bounds of current function")) {
      //We have exited
      handleExit();
    } else if (s.startsWith("No symbol \"")) {
        int head = s.indexOf('\"');
        int tail = s.lastIndexOf('\"');
        QString watchName = s.mid(head+1, tail-head-1);

        // Update current...
        auto result = mWatchVarList.find(watchName);
        if (result != mWatchVarList.end()) {
            PWatchVar watchVar = result.value();
            //todo: update watch value to invalid
            invalidateWatchVar(watchVar);
            watchVar->gdbIndex = -1;
            dorescanwatches = true;
        }
    }
}

void DebugReader::handleExit()
{
    doprocessexited=true;
}

void DebugReader::handleFrames()
{
    QString s = getNextLine();

    // Is this a backtrace dump?
    if (s.startsWith("#")) {
        // Find function name
        if (!findAnnotation(AnnotationType::TFrameFunctionName))
            return;

        PTrace trace = std::make_shared<Trace>();
        trace->funcname = getNextLine();

        // Find argument list start
        if (!findAnnotation(AnnotationType::TFrameArgs))
            return;

        // Arguments are either () or detailed list
        s = getNextLine();

        while (peekNextAnnotation() == AnnotationType::TArgBegin) {

            // argument name
            if (!findAnnotation(AnnotationType::TArgBegin))
                return;

            s = s + getNextLine();

            // =
            if (!findAnnotation(AnnotationType::TArgNameEnd))
                return;
            s = s + ' ' + getNextLine() + ' '; // should be =

            // argument value
            if (!findAnnotation(AnnotationType::TArgValue))
                return;

            s = s + getNextLine();

            // argument end
            if (!findAnnotation(AnnotationType::TArgEnd))
                return;

            s = s + getNextLine();
        }

        trace->funcname = trace->funcname + s.trimmed();

        // source info
        if (peekNextAnnotation() == AnnotationType::TFrameSourceBegin) {
            // Find filename
            if (!findAnnotation(AnnotationType::TFrameSourceFile))
                return;
            trace->filename = getNextLine();
            // find line
            if (!findAnnotation(AnnotationType::TFrameSourceLine))
                return;
            trace->line = getNextLine().trimmed().toInt();
        } else {
            trace->filename = "";
            trace->line = 0;
        }
        mBacktraceModel.addTrace(trace);

        // Skip over the remaining frame part...
        if (!findAnnotation(AnnotationType::TFrameEnd))
            return;

        // Not another one coming? Done!
        if (peekNextAnnotation() != AnnotationType::TFrameBegin) {
            // End of stack trace dump!
              dobacktraceready = true;
        }
    } else
        doupdatecpuwindow = true;
}

void DebugReader::handleLocalOutput()
{
    // name(spaces)hexvalue(tab)decimalvalue
    QString s = getNextFilledLine();

    bool breakLine = false;
    while (true) {
        if (s.startsWith("\032\032")) {
            s = TrimLeft(s);
            if (s == "No locals.") {
                return;
            }
            if (s == "No arguments.") {
                return;
            }
            //todo: update local view
//            if (breakLine and (MainForm.txtLocals.Lines.Count>0) then begin
//          MainForm.txtLocals.Lines[MainForm.txtLocals.Lines.Count-1] := MainForm.txtLocals.Lines[MainForm.txtLocals.Lines.Count-1] + s;
//        end else begin
//          MainForm.txtLocals.Lines.Add(s);
//        end;
            breakLine=false;
        } else {
            breakLine = true;
        }
        s = getNextLine();
        if (!breakLine && s.isEmpty())
            break;
    }
}

void DebugReader::handleLocals()
{
    //todo: clear local view
    handleLocalOutput();
}

void DebugReader::handleParams(){
    handleLocalOutput();
}

void DebugReader::handleRegisters()
{
    // name(spaces)hexvalue(tab)decimalvalue
    QString s = getNextFilledLine();

    while (true) {
        PRegister reg = std::make_shared<Register>();
        // Cut name from 1 to first space
        int x = s.indexOf(' ');
        reg->name = s.mid(0,x-1);
        s.remove(0,x);
        // Remove spaces
        s = TrimLeft(s);

        // Cut hex value from 1 to first tab
        x = s.indexOf('\t');
        if (x<0)
            x = s.indexOf(' ');
        reg->hexValue = s.mid(0,x - 1);
        s.remove(0,x); // delete tab too
        s = TrimLeft(s);

        // Remaining part contains decimal value
        reg->decValue = s;

        mRegisters.append(reg);
        s = getNextLine();
        if (s.isEmpty())
            break;
    }

    doregistersready = true;
}

void DebugReader::handleSignal()
{
    mSignal = getNextFilledLine(); // Program received signal

    if (!findAnnotation(AnnotationType::TSignalName))
        return;

    mSignal = mSignal + getNextFilledLine(); // signal code

    if (!findAnnotation(AnnotationType::TSignalNameEnd))
        return;

    mSignal = mSignal + getNextFilledLine(); // comma

    if (!findAnnotation(AnnotationType::TSignalString))
        return;

    mSignal = mSignal + getNextFilledLine(); // user friendly description

    if (!findAnnotation(AnnotationType::TSignalStringEnd))
        return;

    mSignal = mSignal + getNextFilledLine(); // period

    doreceivedsignal = true;
}

void DebugReader::handleSource()
{
    // source filename:line:offset:beg/middle/end:addr
    QString s = TrimLeft(getRemainingLine());

    // remove offset, beg/middle/end, address
    for (int i=0;i<3;i++) {
        int delimPos = s.lastIndexOf(':');
        if (delimPos >= 0)
            s.remove(delimPos,INT_MAX);
        else
            return; // Wrong format. Don't bother to continue
    }

    // get line
    int delimPos = s.lastIndexOf(':');
    if (delimPos >= 0) {
        mBreakPointLine = s.mid(delimPos+1).toInt();
        s.remove(delimPos, INT_MAX);
    }

    // get file
    mBreakPointFile = s;

    doupdateexecution = true;
    doupdatecpuwindow = true;
}

void DebugReader::handleValueHistoryValue()
{
    mEvalValue = processEvalOutput();
    doevalready = true;
}

AnnotationType DebugReader::peekNextAnnotation()
{
    int indexBackup = mIndex; // do NOT modifiy curpos
    AnnotationType result = getNextAnnotation();
    mIndex = indexBackup;
    return result;
}

void DebugReader::processDebugOutput()
{
    // Only update once per update at most
    //WatchView.Items.BeginUpdate;

    if (mInvalidateAllVars) {
         //invalidate all vars when there's first output
         invalidateAllVars();
         mInvalidateAllVars = false;
    }

    emit parseStarted();

   //try

   dobacktraceready = false;
   dodisassemblerready = false;
   doregistersready = false;
   dorescanwatches = false;
   doevalready = false;
   doprocessexited = false;
   doupdateexecution = false;
   doreceivedsignal = false;
   doupdatecpuwindow = false;
   doreceivedsfwarning = false;

   // Global checks
   if (mOutput.indexOf("warning: Source file is more recent than executable.") >= 0)
       doreceivedsfwarning = true;

   mIndex = 0;
   AnnotationType nextAnnotation;
   do {
       nextAnnotation = getNextAnnotation();
       switch(nextAnnotation) {
       case AnnotationType::TValueHistoryValue:
           handleValueHistoryValue();
           break;
       case AnnotationType::TSignal:
           handleSignal();
           break;
       case AnnotationType::TExit:
           handleExit();
           break;
       case AnnotationType::TFrameBegin:
           handleFrames();
           break;
       case AnnotationType::TInfoAsm:
           handleDisassembly();
           break;
       case AnnotationType::TInfoReg:
           handleRegisters();
           break;
       case AnnotationType::TLocal:
           handleLocals();
           break;
       case AnnotationType::TParam:
           handleParams();
           break;
       case AnnotationType::TErrorBegin:
           handleError();
           break;
       case AnnotationType::TDisplayBegin:
           handleDisplay();
           break;
       case AnnotationType::TSource:
           handleSource();
           break;
       }
   } while (nextAnnotation != AnnotationType::TEOF);

     // Only update once per update at most
   //finally
     //WatchView.Items.EndUpdate;
   //end;

   emit parseFinished();
}

QString DebugReader::processEvalOutput()
{
    int indent = 0;

    // First line gets special treatment
    QString result = getNextLine();
    if (result.startsWith('{'))
        indent+=4;

    // Collect all data, add formatting in between
    AnnotationType nextAnnotation;
    QString nextLine;
    bool shouldExit = false;
    do {
        nextAnnotation = getNextAnnotation();
        nextLine = getNextLine();
        switch(nextAnnotation) {
        // Change indent if { or } is found
        case AnnotationType::TFieldBegin:
            result += "\r\n" + QString(4,' ');
            break;
        case AnnotationType::TFieldValue:
            if (nextLine.startsWith('{') && (peekNextAnnotation() !=
                                             AnnotationType::TArrayBegin))
                indent+=4;
            break;
        case AnnotationType::TFieldEnd:
            if (nextLine.endsWith('}')) {
                indent-=4;
                result += "\r\n" + QString(4,' ');
            }
            break;
        case AnnotationType::TEOF:
        case AnnotationType::TValueHistoryEnd:
        case AnnotationType::TDisplayEnd:
            shouldExit = true;
        }
        result += nextLine;
    } while (!shouldExit);
}

void DebugReader::processWatchOutput(PWatchVar WatchVar)
{
    //todo
}

void DebugReader::runNextCmd()
{
    bool doUpdate=false;

    auto action = finally([this,&doUpdate] {
        if (doUpdate) {
            emit updateWatch();
        }
    });
    QMutexLocker locker(&mCmdQueueMutex);
    if (mCmdQueue.isEmpty()) {
        if ((mCurrentCmd) && (mCurrentCmd->updateWatch)) {
            doUpdate=true;
            if (mUpdateCount>0) {
                mUpdateCount=0;
            }
        }
        return;
    }

    if (mCurrentCmd) {
        mCurrentCmd.reset();
    }

    PDebugCommand pCmd = mCmdQueue.dequeue();
    mCmdRunning = true;
    mCurrentCmd = pCmd;

    QByteArray s;
    s=pCmd->command.toLocal8Bit();
    if (!pCmd->params.isEmpty()) {
        s+=' '+pCmd->params.toLocal8Bit();
    }
    s+= "\n";
    if (mProcess.write(s)<0) {
        emit writeToDebugFailed();
    }

//  if devDebugger.ShowCommandLog or pCmd^.ShowInConsole then begin
    if (true || pCmd->showInConsole) {
        //update debug console
        // if not devDebugger.ShowAnnotations then begin
        if (true) {
//            if MainForm.DebugOutput.Lines.Count>0 then begin
//              MainForm.DebugOutput.Lines.Delete(MainForm.DebugOutput.Lines.Count-1);
//            end;
//            MainForm.DebugOutput.Lines.Add('(gdb)'+pCmd^.Cmd + ' ' + pCmd^.params);
//            MainForm.DebugOutput.Lines.Add('');
        } else {
//            MainForm.DebugOutput.Lines.Add(pCmd^.Cmd + ' ' + pCmd^.params);
//            MainForm.DebugOutput.Lines.Add('');
        }
    }
}

void DebugReader::skipSpaces()
{
    while (mIndex < mOutput.length() &&
           (mOutput[mIndex]=='\t' || mOutput[mIndex]==' '))
        mIndex++;
}

void DebugReader::skipToAnnotation()
{
    // Walk up to the next annotation
    while (mIndex < mOutput.length() &&
           (mOutput[mIndex]!=26))
        mIndex++;
    // Crawl through the remaining ->'s
    while (mIndex < mOutput.length() &&
           (mOutput[mIndex]==26))
        mIndex++;
}

void DebugReader::run()
{
    mStop = false;
    bool errorOccurred = false;
    mProcess.setProgram(cmd);
    mProcess.setArguments(QProcess::splitCommand(arguments));
    mProcess.setWorkingDirectory(workingDir);

    mProcess.connect(&mProcess, &QProcess::errorOccurred,
                    [&](){
                        errorOccurred= true;
                    });
//    mProcess.connect(&process, &QProcess::readyReadStandardError,[&process,this](){
//        this->error(QString::fromLocal8Bit( process.readAllStandardError()));
//    });
//    mProcess.connect(&mProcess, &QProcess::readyReadStandardOutput,[&process,this](){
//        this->log(QString::fromLocal8Bit( process.readAllStandardOutput()));
//    });
//    process.connect(&mProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),[&process,this](){
//        this->error(COMPILE_PROCESS_END);
//    });
    mProcess.start();
    mProcess.waitForStarted(5000);
    QByteArray buffer;
    while (true) {
        mProcess.waitForFinished(1000);
        if (mProcess.state()!=QProcess::Running) {
            break;
        }
        if (mStop) {
            mProcess.terminate();
        }
        if (errorOccurred)
            break;
        buffer += mProcess.readAll();
        if (getLastAnnotation(buffer) == AnnotationType::TPrompt) {
            mOutput = buffer;
            processDebugOutput();
            mCmdRunning = false;
            runNextCmd();
        }
    }
    if (errorOccurred) {
        emit processError(mProcess.error());
    }
}



int BreakpointModel::rowCount(const QModelIndex &) const
{
    return mList.size();
}

int BreakpointModel::columnCount(const QModelIndex &) const
{
    return 3;
}

QVariant BreakpointModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row()<0 || index.row() >= static_cast<int>(mList.size()))
        return QVariant();
    PBreakpoint breakpoint = mList[index.row()];
    if (!breakpoint)
        return QVariant();
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return breakpoint->filename;
        case 1:
            if (breakpoint->line>0)
                return breakpoint->line;
            else
                return "";
        case 2:
            return breakpoint->condition;
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

QVariant BreakpointModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role ==  Qt::DisplayRole) {
        switch(section) {
        case 0:
            return tr("Filename");
        case 1:
            return tr("Line");
        case 2:
            return tr("Condition");
        }
    }
    return QVariant();
}

void BreakpointModel::addBreakpoint(PBreakpoint p)
{
    beginInsertRows(QModelIndex(),mList.size(),mList.size());
    mList.push_back(p);
    endInsertRows();
}

void BreakpointModel::clear()
{
    beginRemoveRows(QModelIndex(),0,mList.size()-1);
    mList.clear();
    endRemoveRows();
}

void BreakpointModel::removeBreakpoint(int row)
{
    beginRemoveRows(QModelIndex(),row,row);
    mList.removeAt(row);
    endRemoveRows();
}


int BacktraceModel::rowCount(const QModelIndex &) const
{
    return mList.size();
}

int BacktraceModel::columnCount(const QModelIndex &) const
{
    return 3;
}

QVariant BacktraceModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row()<0 || index.row() >= static_cast<int>(mList.size()))
        return QVariant();
    PTrace trace = mList[index.row()];
    if (!trace)
        return QVariant();
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return trace->funcname;
        case 1:
            return trace->filename;
        case 2:
            if (trace->line>0)
                return trace->line;
            else
                return "";
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

QVariant BacktraceModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role ==  Qt::DisplayRole) {
        switch(section) {
        case 0:
            return tr("Function");
        case 1:
            return tr("Filename");
        case 2:
            return tr("Line");
        }
    }
    return QVariant();
}

void BacktraceModel::addTrace(PTrace p)
{
    beginInsertRows(QModelIndex(),mList.size(),mList.size());
    mList.push_back(p);
    endInsertRows();
}

void BacktraceModel::clear()
{
    beginRemoveRows(QModelIndex(),0,mList.size()-1);
    mList.clear();
    endRemoveRows();
}

void BacktraceModel::removeTrace(int row)
{
    beginRemoveRows(QModelIndex(),row,row);
    mList.removeAt(row);
    endRemoveRows();
}
