#include "debugger.h"
#include "utils.h"
#include "mainwindow.h"
#include "editor.h"
#include "settings.h"
#include "widgets/cpudialog.h"
#include "systemconsts.h"

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QDebug>

Debugger::Debugger(QObject *parent) : QObject(parent)
{
    mBreakpointModel=new BreakpointModel(this);
    mBacktraceModel=new BacktraceModel(this);
    mWatchModel = new WatchModel(this);
    mRegisterModel = new RegisterModel(this);
    mExecuting = false;
    mUseUTF8 = false;
    mReader = nullptr;
    mCommandChanged = false;
    mLeftPageIndexBackup = -1;
}

bool Debugger::start()
{
    Settings::PCompilerSet compilerSet = pSettings->compilerSets().defaultSet();
    if (!compilerSet) {
        QMessageBox::critical(pMainWindow,
                              tr("No compiler set"),
                              tr("No compiler set is configured.")+tr("Can't start debugging."));
        return false;
    }
    mExecuting = true;
    QString debuggerPath = compilerSet->debugger();
    //QFile debuggerProgram(debuggerPath);
    if (!isTextAllAscii(debuggerPath)) {
        mExecuting = false;
        QMessageBox::critical(pMainWindow,
                              tr("Debugger path error"),
                              tr("Debugger's path \"%1\" contains non-ascii characters.")
                              .arg(debuggerPath)
                              + "<br />"
                              + tr("This prevents it from executing."));
        return false;
    }
    if (!fileExists(debuggerPath)) {
        mExecuting = false;
        QMessageBox::critical(pMainWindow,
                              tr("Debugger not exists"),
                              tr("Can''t find debugger in : \"%1\"").arg(debuggerPath));
        return false;
    }
    mReader = new DebugReader(this);
    mReader->setDebuggerPath(debuggerPath);
    connect(mReader, &QThread::finished,this,&Debugger::clearUpReader);
    connect(mReader, &DebugReader::parseFinished,this,&Debugger::syncFinishedParsing,Qt::BlockingQueuedConnection);
    connect(mReader, &DebugReader::changeDebugConsoleLastLine,this,&Debugger::onChangeDebugConsoleLastline);
    connect(this, &Debugger::localsReady,pMainWindow,&MainWindow::onLocalsReady);
    connect(mReader, &DebugReader::cmdStarted,pMainWindow, &MainWindow::disableDebugActions);
    connect(mReader, &DebugReader::cmdFinished,pMainWindow, &MainWindow::enableDebugActions);

    mReader->start();
    mReader->mStartSemaphore.acquire(1);

    pMainWindow->updateAppTitle();

    //Application.HintHidePause := 5000;
    return true;
}
void Debugger::stop() {
    if (mExecuting) {
        mReader->stopDebug();
    }
}
void Debugger::clearUpReader()
{
    if (mExecuting) {
        mExecuting = false;

        //stop debugger
        mReader->deleteLater();
        mReader=nullptr;

//        if WatchVarList.Count = 0 then // nothing worth showing, restore view
//          MainForm.LeftPageControl.ActivePageIndex := LeftPageIndexBackup;

//        // Close CPU window
        if (pMainWindow->cpuDialog()!=nullptr) {
            pMainWindow->cpuDialog()->close();
        }

        // Free resources
        pMainWindow->removeActiveBreakpoints();

        pMainWindow->txtLocals()->clear();

        pMainWindow->updateAppTitle();

        pMainWindow->updateDebugEval("");

        mBacktraceModel->clear();

        for(PWatchVar var:mWatchModel->watchVars()) {
            invalidateWatchVar(var);
        }

        pMainWindow->updateEditorActions();
    }
}

RegisterModel *Debugger::registerModel() const
{
    return mRegisterModel;
}

WatchModel *Debugger::watchModel() const
{
    return mWatchModel;
}

void Debugger::sendCommand(const QString &command, const QString &params, bool updateWatch, bool showInConsole, DebugCommandSource source)
{
    if (mExecuting && mReader) {
        mReader->postCommand(command,params,updateWatch,showInConsole,source);
    }
}

bool Debugger::commandRunning()
{
    if (mExecuting && mReader) {
        return mReader->commandRunning();
    }
    return false;
}

void Debugger::addBreakpoint(int line, const Editor* editor)
{
    addBreakpoint(line,editor->filename());
}

void Debugger::addBreakpoint(int line, const QString &filename)
{
    PBreakpoint bp=std::make_shared<Breakpoint>();
    bp->line = line;
    bp->filename = filename;
    bp->condition = "";
    mBreakpointModel->addBreakpoint(bp);
    if (mExecuting) {
        sendBreakpointCommand(bp);
    }
}

void Debugger::deleteBreakpoints(const QString &filename)
{
    for (int i=mBreakpointModel->breakpoints().size()-1;i>=0;i--) {
        PBreakpoint bp = mBreakpointModel->breakpoints()[i];
        if (bp->filename == filename) {
            mBreakpointModel->removeBreakpoint(i);
        }
    }
}

void Debugger::deleteBreakpoints(const Editor *editor)
{
    deleteBreakpoints(editor->filename());
}

void Debugger::deleteBreakpoints()
{
    for (int i=mBreakpointModel->breakpoints().size()-1;i>=0;i--) {
        removeBreakpoint(i);
    }
}

void Debugger::removeBreakpoint(int line, const Editor *editor)
{
    removeBreakpoint(line,editor->filename());
}

void Debugger::removeBreakpoint(int line, const QString &filename)
{
    for (int i=mBreakpointModel->breakpoints().size()-1;i>=0;i--) {
        PBreakpoint bp = mBreakpointModel->breakpoints()[i];
        if (bp->filename == filename && bp->line == line) {
            removeBreakpoint(i);
        }
    }
}

void Debugger::removeBreakpoint(int index)
{
    sendClearBreakpointCommand(index);
    mBreakpointModel->removeBreakpoint(index);
}

PBreakpoint Debugger::breakpointAt(int line, const QString& filename, int &index)
{
    const QList<PBreakpoint>& breakpoints=mBreakpointModel->breakpoints();
    for (index=0;index<breakpoints.count();index++){
        PBreakpoint breakpoint = breakpoints[index];
        if (breakpoint->line == line
                && breakpoint->filename == filename)
            return breakpoint;
    }
    index=-1;
    return PBreakpoint();
}

PBreakpoint Debugger::breakpointAt(int line, const Editor *editor, int &index)
{
    return breakpointAt(line,editor->filename(),index);
}

void Debugger::setBreakPointCondition(int index, const QString &condition)
{
    PBreakpoint breakpoint=mBreakpointModel->setBreakPointCondition(index,condition);
    if (condition.isEmpty()) {
        sendCommand("cond",
                    QString("%1").arg(breakpoint->line));
    } else {
        sendCommand("cond",
                    QString("%1 %2").arg(breakpoint->line).arg(condition));
    }
}

void Debugger::sendAllBreakpointsToDebugger()
{
    for (PBreakpoint breakpoint:mBreakpointModel->breakpoints()) {
        sendBreakpointCommand(breakpoint);
    }
}

void Debugger::addWatchVar(const QString &namein)
{
    // Don't allow duplicates...
    PWatchVar oldVar = mWatchModel->findWatchVar(namein);
    if (oldVar)
        return;

    PWatchVar var = std::make_shared<WatchVar>();
    var->parent= nullptr;
    var->name = namein;
    var->value = tr("Execute to evaluate");
    var->gdbIndex = -1;

    mWatchModel->addWatchVar(var);
    sendWatchCommand(var);
}

void Debugger::renameWatchVar(const QString &oldname, const QString &newname)
{
    // check if name already exists;
    PWatchVar var = mWatchModel->findWatchVar(newname);
    if (var)
        return;

    var = mWatchModel->findWatchVar(oldname);
    if (var) {
        var->name = newname;
        if (mExecuting && var->gdbIndex!=-1)
            sendRemoveWatchCommand(var);
        invalidateWatchVar(var);

        if (mExecuting) {
            sendWatchCommand(var);
        }
    }
}

void Debugger::refreshWatchVars()
{
    for (PWatchVar var:mWatchModel->watchVars()) {
        if (var->gdbIndex == -1)
            sendWatchCommand(var);
    }
}

void Debugger::removeWatchVars(bool deleteparent)
{
    if (deleteparent) {
        mWatchModel->clear();
    } else {
        for(const PWatchVar& var:mWatchModel->watchVars()) {
            sendRemoveWatchCommand(var);
            invalidateWatchVar(var);
        }
    }
}

void Debugger::removeWatchVar(const QModelIndex &index)
{
    mWatchModel->removeWatchVar(index);
}

void Debugger::invalidateAllVars()
{
    mReader->setInvalidateAllVars(true);
}

void Debugger::sendAllWatchvarsToDebugger()
{
    for (PWatchVar var:mWatchModel->watchVars()) {
        sendWatchCommand(var);
    }
}

void Debugger::invalidateWatchVar(const QString &name)
{
    PWatchVar var = mWatchModel->findWatchVar(name);
    if (var) {
        invalidateWatchVar(var);
    }
}

void Debugger::invalidateWatchVar(PWatchVar var)
{
    var->gdbIndex = -1;
    QString value;
    if (mExecuting) {
        value = tr("Not found in current context");
    } else {
        value = tr("Execute to evaluate");
    }
    var->value = value;
    if (var->children.isEmpty()) {
        mWatchModel->notifyUpdated(var);
    } else {
        mWatchModel->beginUpdate();
        var->children.clear();
        mWatchModel->endUpdate();
    }
}

PWatchVar Debugger::findWatchVar(const QString &name)
{
    return mWatchModel->findWatchVar(name);
}

//void Debugger::notifyWatchVarUpdated(PWatchVar var)
//{
//    mWatchModel->notifyUpdated(var);
//}

void Debugger::notifyBeforeProcessWatchVar()
{
    mWatchModel->beginUpdate();
}

void Debugger::notifyAfterProcessWatchVar()
{
    mWatchModel->endUpdate();
}

void Debugger::updateDebugInfo()
{
    sendCommand("backtrace", "");
    sendCommand("info locals", "");
    sendCommand("info args", "");
}

bool Debugger::useUTF8() const
{
    return mUseUTF8;
}

void Debugger::setUseUTF8(bool useUTF8)
{
    mUseUTF8 = useUTF8;
}

BacktraceModel* Debugger::backtraceModel()
{
    return mBacktraceModel;
}

BreakpointModel *Debugger::breakpointModel()
{
    return mBreakpointModel;
}

void Debugger::sendWatchCommand(PWatchVar var)
{
    sendCommand("display", var->name);
}

void Debugger::sendRemoveWatchCommand(PWatchVar var)
{
    sendCommand("undisplay",QString("%1").arg(var->gdbIndex));
}

void Debugger::sendBreakpointCommand(PBreakpoint breakpoint)
{
    if (breakpoint && mExecuting) {
        // break "filename":linenum
        QString condition;
        if (!breakpoint->condition.isEmpty()) {
            condition = " if " + breakpoint->condition;
        }
        QString filename = breakpoint->filename;
        filename.replace('\\','/');
        sendCommand("break",
                    QString("\"%1\":%2").arg(filename)
                    .arg(breakpoint->line)+condition);
    }
}

void Debugger::sendClearBreakpointCommand(int index)
{
    sendClearBreakpointCommand(mBreakpointModel->breakpoints()[index]);
}

void Debugger::sendClearBreakpointCommand(PBreakpoint breakpoint)
{
    // Debugger already running? Remove it from GDB
    if (breakpoint && mExecuting) {
        //clear "filename":linenum
        QString filename = breakpoint->filename;
        filename.replace('\\','/');
        sendCommand("clear",
                QString("\"%1\":%2").arg(filename)
                .arg(breakpoint->line));
    }
}

void Debugger::syncFinishedParsing()
{
    bool spawnedcpuform = false;

    // GDB determined that the source code is more recent than the executable. Ask the user if he wants to rebuild.
    if (mReader->doreceivedsfwarning) {
        if (QMessageBox::question(pMainWindow,
                                  tr("Compile"),
                                  tr("Source file is more recent than executable.")+"<BR /><BR />" + tr("Recompile?"),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::Yes
                                  ) == QMessageBox::Yes) {
            stop();
            pMainWindow->compile();
            return;
        }
    }

    // The program to debug has stopped. Stop the debugger
    if (mReader->doprocessexited) {
        stop();
        return;
    }

    // An evaluation variable has been processed. Forward the results
    if (mReader->doevalready) {
        //pMainWindow->updateDebugEval(mReader->mEvalValue);
        emit evalValueReady(mReader->mEvalValue);
        mReader->mEvalValue="";
        mReader->doevalready = false;
    }

    if (mReader->doupdatememoryview) {
        emit memoryExamineReady(mReader->mMemoryValue);
        mReader->mMemoryValue.clear();
        mReader->doupdatememoryview=false;
    }

    if (mReader->doupdatelocal) {
        emit localsReady(mReader->mLocalsValue);
        mReader->mLocalsValue.clear();
        mReader->doupdatelocal=false;
    }

    // show command output
    if (pSettings->debugger().showCommandLog() ||
            (mReader->mCurrentCmd && mReader->mCurrentCmd->showInConsole)) {
        if (pSettings->debugger().showAnnotations()) {
            QString strOutput = mReader->mOutput;
            strOutput.replace(QChar(26),'>');
            pMainWindow->addDebugOutput(strOutput);
            pMainWindow->addDebugOutput("");
            pMainWindow->addDebugOutput("");
        } else {
            QStringList strList = TextToLines(mReader->mOutput);
            QStringList outStrList;
            bool addToLastLine=false;
            for (int i=0;i<strList.size();i++) {
                QString strOutput=strList[i];
                if (strOutput.startsWith("\032\032")) {
                    addToLastLine = true;
                } else {
                    if (addToLastLine && outStrList.size()>0) {
                        outStrList[outStrList.size()-1]+=strOutput;
                    } else {
                        outStrList.append(strOutput);
                    }
                    addToLastLine = false;
                }
            }
            for (const QString& line:outStrList) {
                pMainWindow->addDebugOutput(line);
            }
        }
    }

    // Some part of the CPU form has been updated
    if (pMainWindow->cpuDialog()!=nullptr && !mReader->doreceivedsignal) {
        if (mReader->doregistersready) {
            mRegisterModel->update(mReader->mRegisters);
            mReader->mRegisters.clear();
            mReader->doregistersready = false;
        }

        if (mReader->dodisassemblerready) {
            pMainWindow->cpuDialog()->setDisassembly(mReader->mDisassembly);
            mReader->mDisassembly.clear();
            mReader->dodisassemblerready = false;
        }
    }

    if (mReader->doupdateexecution) {
        if (mReader->mCurrentCmd && mReader->mCurrentCmd->source == DebugCommandSource::Console) {
            pMainWindow->setActiveBreakpoint(mReader->mBreakPointFile, mReader->mBreakPointLine,false);
        } else {
            pMainWindow->setActiveBreakpoint(mReader->mBreakPointFile, mReader->mBreakPointLine);
        }
        refreshWatchVars(); // update variable information
    }

    if (mReader->doreceivedsignal) {
//SignalDialog := CreateMessageDialog(fSignal, mtError, [mbOk]);
//SignalCheck := TCheckBox.Create(SignalDialog);

//// Display it on top of everything
//SignalDialog.FormStyle := fsStayOnTop;

//SignalDialog.Height := 150;

//with SignalCheck do begin
//  Parent := SignalDialog;
//  Caption := 'Show CPU window';
//  Top := Parent.ClientHeight - 22;
//  Left := 8;
//  Width := Parent.ClientWidth - 16;
//  Checked := devData.ShowCPUSignal;
//end;

//MessageBeep(MB_ICONERROR);
//if SignalDialog.ShowModal = ID_OK then begin
//  devData.ShowCPUSignal := SignalCheck.Checked;
//  if SignalCheck.Checked and not Assigned(CPUForm) then begin
//    MainForm.ViewCPUItemClick(nil);
//    spawnedcpuform := true;
//  end;
//end;

//SignalDialog.Free;

    }


    // CPU form updates itself when spawned, don't update twice!
    if ((mReader->doupdatecpuwindow && !spawnedcpuform) && (pMainWindow->cpuDialog()!=nullptr)) {
        pMainWindow->cpuDialog()->updateInfo();
    }
}

void Debugger::onChangeDebugConsoleLastline(const QString &text)
{
    //pMainWindow->changeDebugOutputLastline(text);
    pMainWindow->addDebugOutput(text);
}

int Debugger::leftPageIndexBackup() const
{
    return mLeftPageIndexBackup;
}

void Debugger::setLeftPageIndexBackup(int leftPageIndexBackup)
{
    mLeftPageIndexBackup = leftPageIndexBackup;
}

bool Debugger::executing() const
{
    return mExecuting;
}

DebugReader::DebugReader(Debugger* debugger, QObject *parent) : QThread(parent),
    mStartSemaphore(0)
{
    mDebugger = debugger;
    mProcess = nullptr;
    mUseUTF8 = false;
    mCmdRunning = false;
    mInvalidateAllVars = false;
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
//    if (!mCmdRunning)
//        runNextCmd();
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
        //hack to catch local
        if ((mCurrentCmd) && (mCurrentCmd->command == "info locals")) {
            result = AnnotationType::TLocal;
        } else if ((mCurrentCmd) && (mCurrentCmd->command == "info args")) {
            //hack to catch params
            result = AnnotationType::TParam;
        } else if ((mCurrentCmd) && (mCurrentCmd->command == "info") && (mCurrentCmd->params=="registers")) {
            // Hack fix to catch register dump
            result = AnnotationType::TInfoReg;
        } else if ((mCurrentCmd) && (mCurrentCmd->command == "disas")) {
            // Another hack to catch assembler            
            result = AnnotationType::TInfoAsm;
        } else if ((mCurrentCmd) && (mCurrentCmd->command.startsWith("x/"))) {
            result = AnnotationType::TMemory;
        }
        return result;
    } else if (s == "error") {
        return AnnotationType::TError;
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
    while (mIndex<mOutput.length() && mOutput[mIndex]!='\r' && mOutput[mIndex]!='\n' && mOutput[mIndex]!=0)
        mIndex++;
    // Skip enter sequences (CRLF, CR, LF, etc.)
    while (mIndex<mOutput.length() && mOutput[mIndex]=='\r' && mOutput[mIndex]=='\n' && mOutput[mIndex]==0)
        mIndex++;
    // Return next line
    return getRemainingLine();
}

QString DebugReader::getNextLine()
{
    // Walk up to an enter sequence
    while (mIndex<mOutput.length() && mOutput[mIndex]!='\r' && mOutput[mIndex]!='\n' && mOutput[mIndex]!=0)
        mIndex++;

    // End of output. Exit
    if (mIndex>=mOutput.length())
        return "";
    // Skip ONE enter sequence (CRLF, CR, LF, etc.)
    if ((mOutput[mIndex] == '\r') && (mIndex+1<mOutput.length()) &&  (mOutput[mIndex+1] == '\n')) // DOS
        mIndex+=2;
    else if (mOutput[mIndex] == '\r')  // UNIX
        mIndex++;
    else if (mOutput[mIndex] == '\n') // MAC
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
    while (mIndex<mOutput.length() && mOutput[mIndex]!='\r' && mOutput[mIndex]!='\n' && mOutput[mIndex]!=0) {
        Result += mOutput[mIndex];
        mIndex++;
    }
    return Result;
}

void DebugReader::handleDisassembly()
{
    // Get info message
    QString s = getNextLine();

    // the full function name will be saved at index 0
    mDisassembly.append(s.mid(36));

    s = getNextLine();

    // Add lines of disassembly
    while (s != "End of assembler dump.") {
        if(!s.isEmpty())
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
    // Find watchVar we're talking about
    PWatchVar watchVar = mDebugger->findWatchVar(watchName);
    if (watchVar) {
        // Advance up to the value
        if (!findAnnotation(AnnotationType::TDisplayExpression))
            return;;
        // Refresh GDB index so we can undisplay this by index
        watchVar->gdbIndex = s.toInt();
        mDebugger->notifyBeforeProcessWatchVar();
        processWatchOutput(watchVar);
        mDebugger->notifyAfterProcessWatchVar();
        //mDebugger->notifyWatchVarUpdated(watchVar);
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

        // Update current..
        mDebugger->invalidateWatchVar(watchName);
    }
}

void DebugReader::handleErrorExit()
{
    if ((mCurrentCmd) && (
                mCurrentCmd->command == "next"
                || mCurrentCmd->command == "step"
                || mCurrentCmd->command == "finish"
                || mCurrentCmd->command == "continue")) {
        handleExit();
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
        if (s.startsWith("#0")) {
            mDebugger->backtraceModel()->clear();
        }
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
        mDebugger->backtraceModel()->addTrace(trace);

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

    bool nobreakLine = false;
    QString line;
    while (true) {
        if (!s.startsWith("\032\032")) {
            s = TrimLeft(s);
            if (s == "No locals.") {
                return;
            }
            if (s == "No arguments.") {
                return;
            }
            //todo: update local view
            if (nobreakLine && pMainWindow->txtLocals()->document()->lineCount()>0) {
                line += s;
//                emit addLocalWithoutLinebreak(s);
            } else {
                mLocalsValue.append(line);
                line = s;
            }
            nobreakLine=false;
        } else {
            nobreakLine = true;
        }
        s = getNextLine();
        if (!nobreakLine && s.isEmpty())
            break;
    }
    if (!line.isEmpty()) {
        mLocalsValue.append(line);
    }
}

void DebugReader::handleLocals()
{
    mLocalsValue.clear();
    handleLocalOutput();
}

void DebugReader::handleMemory()
{
    doupdatememoryview = true;
    // name(spaces)hexvalue(tab)decimalvalue
    mMemoryValue.clear();
    QString s = getNextFilledLine();
    bool isAnnotation = false;
    while (true) {
        if (!s.startsWith("\032\032")) {
            s = s.trimmed();
            if (!s.isEmpty()) {
                mMemoryValue.append(s);
            }
            isAnnotation = false;
        } else {
            isAnnotation = true;
        }
        s = getNextLine();
        if (!isAnnotation && s.isEmpty())
            break;
    }
}

void DebugReader::handleParams(){
    handleLocalOutput();
    doupdatelocal = true;
}

void DebugReader::handleRegisters()
{
    // name(spaces)hexvalue(tab)decimalvalue
    QString s = getNextFilledLine();

    while (true) {
        PRegister reg = std::make_shared<Register>();
        // Cut name from 1 to first space
        int x = s.indexOf(' ');
        reg->name = s.mid(0,x);
        s.remove(0,x);
        // Remove spaces
        s = TrimLeft(s);

        // Cut hex value from 1 to first tab
        x = s.indexOf('\t');
        if (x<0)
            x = s.indexOf(' ');
        reg->hexValue = s.mid(0,x);
        s.remove(0,x); // delete tab too
        s = TrimLeft(s);

        // Remaining part contains decimal value
        reg->decValue = s;

        if (!reg->name.trimmed().isEmpty())
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
         mDebugger->removeWatchVars(false);
         mInvalidateAllVars = false;
    }

    emit parseStarted();

   //try

   dobacktraceready = false;
   dodisassemblerready = false;
   doregistersready = false;
   doevalready = false;
   doupdatememoryview = false;
   doupdatelocal = false;
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
       case AnnotationType::TError:
           handleErrorExit();
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
       case AnnotationType::TMemory:
           handleMemory();
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
    return result;
}

void DebugReader::processWatchOutput(PWatchVar watchVar)
{
//    // Expand if it was expanded or if it didn't have any children
//    bool ParentWasExpanded = false;

    // Do not remove root node of watch variable

    watchVar->children.clear();
    watchVar->value = "";
    // Process output parsed by ProcessEvalStruct
    QString s = processEvalOutput();

    QStringList tokens = tokenize(s);
    PWatchVar parentVar = watchVar;
    PWatchVar currentVar = watchVar;

    QVector<PWatchVar> varStack;
    int i=0;
    while (i<tokens.length()) {
        QString token = tokens[i];
        QChar ch = token[0];
        if (ch =='_' || (ch>='a' && ch<='z')
                || (ch>='A' && ch<='Z') || (ch>127)) {
            //is identifier,create new child node
            PWatchVar newVar = std::make_shared<WatchVar>();
            newVar->parent = parentVar.get();
            newVar->name = token;
            newVar->fullName = parentVar->fullName + '.'+token;
            newVar->value = "";
            newVar->gdbIndex = -1;
            parentVar->children.append(newVar);
            currentVar = newVar;
        } else if (ch == '{') {
            if (parentVar->value.isEmpty()) {
                parentVar->value = "{";
            } else {
                PWatchVar newVar = std::make_shared<WatchVar>();
                newVar->parent = parentVar.get();
                if (parentVar) {
                    int count = parentVar->children.count();
                    newVar->name = QString("[%1]").arg(count);
                    newVar->fullName = parentVar->fullName + newVar->name;
                } else {
                    newVar->name = QString("[0]");
                    newVar->fullName = newVar->name;
                }
                newVar->value = "{";
                parentVar->children.append(newVar);
                varStack.push_back(parentVar);
                parentVar = newVar;
            }
            currentVar = nullptr;
        } else if (ch == '}') {
            currentVar = nullptr;
            PWatchVar newVar = std::make_shared<WatchVar>();
            newVar->parent = parentVar.get();
            newVar->name = "";
            newVar->value = "}";
            newVar->gdbIndex = -1;
            parentVar->children.append(newVar);
            if (!varStack.isEmpty()) {
                parentVar = varStack.back();
                varStack.pop_back();
            }
        } else if (ch == '=') {
            // just skip it
        } else if (ch == ',') {
                currentVar = nullptr;
        } else {
            if (currentVar) {
                if (currentVar->value.isEmpty()) {
                    currentVar->value = token;
                } else {
                    currentVar->value += " "+token;
                }
            } else {
                PWatchVar newVar = std::make_shared<WatchVar>();
                newVar->parent = parentVar.get();
                newVar->name = QString("[%1]")
                        .arg(parentVar->children.count());
                newVar->fullName = parentVar->fullName + newVar->name;
                newVar->value = token;
                newVar->gdbIndex = -1;
                parentVar->children.append(newVar);
            }
        }
        i++;
    }
    // add placeholder name for variable name so we can format structs using one rule

    // Add children based on indent
//    QStringList lines = TextToLines(s);

//    for (const QString& line:lines) {
//        // Format node text. Remove trailing comma
//        QString nodeText = line.trimmed();
//        if (nodeText.endsWith(',')) {
//            nodeText.remove(nodeText.length()-1,1);
//        }

//        if (nodeText.endsWith('{')) { // new member struct
//            if (parentVar->text.isEmpty()) { // root node, replace text only
//                parentVar->text = nodeText;
//            } else {
//                PWatchVar newVar = std::make_shared<WatchVar>();
//                newVar->parent = parentVar.get();
//                newVar->name = "";
//                newVar->text = nodeText;
//                newVar->gdbIndex = -1;
//                parentVar->children.append(newVar);
//                varStack.push_back(parentVar);
//                parentVar = newVar;
//            }
//        } else if (nodeText.startsWith('}')) { // end of struct, change parent
//                PWatchVar newVar = std::make_shared<WatchVar>();
//                newVar->parent = parentVar.get();
//                newVar->name = "";
//                newVar->text = "}";
//                newVar->gdbIndex = -1;
//                parentVar->children.append(newVar);
//                if (!varStack.isEmpty()) {
//                    parentVar = varStack.back();
//                    varStack.pop_back();
//                }
//        } else { // next parent member/child
//            if (parentVar->text.isEmpty()) { // root node, replace text only
//                parentVar->text = nodeText;
//            } else {
//                PWatchVar newVar = std::make_shared<WatchVar>();
//                newVar->parent = parentVar.get();
//                newVar->name = "";
//                newVar->text = nodeText;
//                newVar->gdbIndex = -1;
//                parentVar->children.append(newVar);
//            }
//        }
//    }
        // TODO: remember expansion state
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
            emit cmdFinished();
        }
        return;
    }

    if (mCurrentCmd) {
        mCurrentCmd.reset();
    }

    PDebugCommand pCmd = mCmdQueue.dequeue();
    mCmdRunning = true;
    mCurrentCmd = pCmd;    
    if (mCurrentCmd->updateWatch)
        emit cmdStarted();

    QByteArray s;
    s=pCmd->command.toLocal8Bit();
    if (!pCmd->params.isEmpty()) {
        s+=' '+pCmd->params.toLocal8Bit();
    }
    s+= "\n";
    if (mProcess->write(s)<0) {
        emit writeToDebugFailed();
    }

//  if devDebugger.ShowCommandLog or pCmd^.ShowInConsole then begin
    if (pSettings->debugger().showCommandLog() || pCmd->showInConsole) {
        //update debug console
        // if not devDebugger.ShowAnnotations then begin
        if (!pSettings->debugger().showAnnotations()) {
//            if MainForm.DebugOutput.Lines.Count>0 then begin
//              MainForm.DebugOutput.Lines.Delete(MainForm.DebugOutput.Lines.Count-1);
//            end;
            emit changeDebugConsoleLastLine("(gdb)"+pCmd->command + ' ' + pCmd->params);
//            MainForm.DebugOutput.Lines.Add('(gdb)'+pCmd^.Cmd + ' ' + pCmd^.params);
//            MainForm.DebugOutput.Lines.Add('');
        } else {
            emit changeDebugConsoleLastLine("(gdb)"+pCmd->command + ' ' + pCmd->params);
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

QStringList DebugReader::tokenize(const QString &s)
{
    QStringList result;
    int tStart,tEnd;
    int i=0;
    while (i<s.length()) {
        QChar ch = s[i];
        if (ch == ' ' || ch == '\t'
                || ch == '\r'
                || ch == '\n') {
//            if (!current.isEmpty()) {
//                result.append(current);
//                current = "";
//            }
            i++;
            continue;
        } else if (ch == '\'') {
            tStart = i;
            i++; //skip \'
            while (i<s.length()) {
                if (s[i]=='\'') {
                    i++;
                    break;
                } else if (s[i] == '\\') {
                    i+=2;
                    continue;
                }
                i++;
            }
            tEnd = std::min(i,s.length());
            result.append(s.mid(tStart,tEnd-tStart));
        } else if (ch == '\"') {
            tStart = i;
            i++; //skip \'
            while (i<s.length()) {
                if (s[i]=='\"') {
                    i++;
                    break;
                } else if (s[i] == '\\') {
                    i+=2;
                    continue;
                }
                i++;
            }
            tEnd = std::min(i,s.length());
            result.append(s.mid(tStart,tEnd-tStart));
        } else if (ch == '<') {
            tStart = i;
            i++;
            while (i<s.length()) {
                if (s[i]=='>') {
                    i++;
                    break;
                }
                i++;
            }
            tEnd = std::min(i,s.length());
            result.append(s.mid(tStart,tEnd-tStart));
        } else if (ch == '(') {
            tStart = i;
            i++;
            while (i<s.length()) {
                if (s[i]==')') {
                    i++;
                    break;
                }
                i++;
            }
            tEnd = std::min(i,s.length());
            result.append(s.mid(tStart,tEnd-tStart));
        } else if (ch == '_' ||
                   ch == '.' ||
                   ch == '+' ||
                   ch == '-' ||
                   ch.isLetterOrNumber() ) {
            tStart = i;
            while (i<s.length()) {
                ch = s[i];
                if (!(ch == '_' ||
                     ch == '.' ||
                     ch == '+' ||
                     ch == '-' ||
                     ch.isLetterOrNumber() ))
                    break;
                i++;
            }
            tEnd = std::min(i,s.length());
            result.append(s.mid(tStart,tEnd-tStart));
        } else {
            result.append(s[i]);
            i++;
        }
    }
    return result;
}

bool DebugReader::invalidateAllVars() const
{
    return mInvalidateAllVars;
}

void DebugReader::setInvalidateAllVars(bool invalidateAllVars)
{
    mInvalidateAllVars = invalidateAllVars;
}

QString DebugReader::debuggerPath() const
{
    return mDebuggerPath;
}

void DebugReader::setDebuggerPath(const QString &debuggerPath)
{
    mDebuggerPath = debuggerPath;
}

void DebugReader::stopDebug()
{
    mStop = true;
}

bool DebugReader::commandRunning()
{
    return !mCmdQueue.isEmpty();
}


void DebugReader::run()
{
    mStop = false;
    bool errorOccurred = false;
    QString cmd = mDebuggerPath;
    QString arguments = "--annotate=2 --silent";
    QString workingDir = QFileInfo(mDebuggerPath).path();

    mProcess = new QProcess();
    mProcess->setProgram(cmd);
    mProcess->setArguments(QProcess::splitCommand(arguments));
    QString cmdDir = extractFileDir(cmd);
    if (!cmdDir.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString path = env.value("PATH");
        if (path.isEmpty()) {
            path = cmdDir;
        } else {
            path = cmdDir + PATH_SEPARATOR + path;
        }
        env.insert("PATH",path);
        mProcess->setProcessEnvironment(env);
    }
    mProcess->setWorkingDirectory(workingDir);

    connect(mProcess, &QProcess::errorOccurred,
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
    mProcess->start();
    mProcess->waitForStarted(5000);
    mStartSemaphore.release(1);
    QByteArray buffer;
    QByteArray readed;
    while (true) {
        mProcess->waitForFinished(1);
        if (mProcess->state()!=QProcess::Running) {
            break;
        }
        if (mStop) {
            mProcess->closeReadChannel(QProcess::StandardOutput);
            mProcess->closeReadChannel(QProcess::StandardError);
            mProcess->closeWriteChannel();
            mProcess->terminate();
            mProcess->kill();
            break;
        }
        if (errorOccurred)
            break;
        readed = mProcess->readAll();
        buffer += readed;
        if (getLastAnnotation(buffer) == AnnotationType::TPrompt) {
            mOutput = QString::fromLocal8Bit(buffer);
            processDebugOutput();
            buffer.clear();
            mCmdRunning = false;
            runNextCmd();
        } else if (!mCmdRunning && readed.isEmpty()){
            runNextCmd();
        } else if (readed.isEmpty()){
            msleep(1);
        }
    }
    if (errorOccurred) {
        emit processError(mProcess->error());
    }
}



BreakpointModel::BreakpointModel(QObject *parent):QAbstractTableModel(parent)
{

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
        case 0: {
            return extractFileName(breakpoint->filename);
        }
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
    case Qt::ToolTipRole:
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

PBreakpoint BreakpointModel::setBreakPointCondition(int index, const QString &condition)
{
    PBreakpoint breakpoint = mList[index];
    breakpoint->condition = condition;
    emit dataChanged(createIndex(index,0),createIndex(index,2));
    return breakpoint;
}

const QList<PBreakpoint> &BreakpointModel::breakpoints() const
{
    return mList;
}

PBreakpoint BreakpointModel::breakpoint(int index) const
{
    if (index<0 && index>=mList.count())
        return PBreakpoint();
    return mList[index];
}

void BreakpointModel::onFileDeleteLines(const QString &filename, int startLine, int count)
{
    beginResetModel();
    for (int i = mList.count()-1;i>=0;i--){
        PBreakpoint breakpoint = mList[i];
        if  (breakpoint->filename == filename
             && breakpoint->line>=startLine) {
            if (breakpoint->line >= startLine+count) {
                breakpoint->line -= count;
            } else {
                mList.removeAt(i);
            }
        }
    }
    endResetModel();
}

void BreakpointModel::onFileInsertLines(const QString &filename, int startLine, int count)
{
    beginResetModel();
    for (int i = mList.count()-1;i>=0;i--){
        PBreakpoint breakpoint = mList[i];
        if  (breakpoint->filename == filename
             && breakpoint->line>=startLine) {
            breakpoint->line+=count;
        }
    }
    endResetModel();
}


BacktraceModel::BacktraceModel(QObject *parent):QAbstractTableModel(parent)
{

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

const QList<PTrace> &BacktraceModel::backtraces() const
{
    return mList;
}

PTrace BacktraceModel::backtrace(int index) const
{
    if (index>=0 && index < mList.count()){
        return mList[index];
    }
    return PTrace();
}

WatchModel::WatchModel(QObject *parent):QAbstractItemModel(parent)
{
    mUpdateCount = 0;
}

QVariant WatchModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }
    WatchVar* item = static_cast<WatchVar*>(index.internalPointer());
    switch (role) {
    case Qt::DisplayRole:
        //qDebug()<<"item->text:"<<item->text;
        switch(index.column()) {
        case 0:
            return item->name;
        case 1:
            return item->value;
        }
    }
    return QVariant();
}

QModelIndex WatchModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row,column,parent))
        return QModelIndex();

    WatchVar* parentItem;
    PWatchVar pChild;
    if (!parent.isValid()) {
        parentItem = nullptr;
        pChild = mWatchVars[row];
    } else {
        parentItem = static_cast<WatchVar*>(parent.internalPointer());
        pChild = parentItem->children[row];
    }
    if (pChild) {
        return createIndex(row,column,pChild.get());
    }
    return QModelIndex();
}

static int getWatchIndex(WatchVar* var, const QList<PWatchVar> list) {
    for (int i=0;i<list.size();i++) {
        PWatchVar v = list[i];
        if (v.get() == var) {
            return i;
        }
    }
    return -1;
}

QModelIndex WatchModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }
    WatchVar* childItem = static_cast<WatchVar*>(index.internalPointer());
    WatchVar* parentItem = childItem->parent;

    //parent is root
    if (parentItem == nullptr) {
        return QModelIndex();
    }
    int row;
    WatchVar* grandItem = parentItem->parent;
    if (grandItem == nullptr) {
        row = getWatchIndex(parentItem,mWatchVars);
    } else {
        row = getWatchIndex(parentItem,grandItem->children);
    }
    return createIndex(row,0,parentItem);
}

int WatchModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return mWatchVars.count();
    } else {
        WatchVar* parentItem = static_cast<WatchVar*>(parent.internalPointer());
        return parentItem->children.count();
    }
}

int WatchModel::columnCount(const QModelIndex&) const
{
    return 2;
}

void WatchModel::addWatchVar(PWatchVar watchVar)
{
    for (PWatchVar var:mWatchVars) {
        if (watchVar->name == var->name) {
            return;
        }
    }
    this->beginInsertRows(QModelIndex(),mWatchVars.size(),mWatchVars.size());
    mWatchVars.append(watchVar);
    this->endInsertRows();
}

void WatchModel::removeWatchVar(const QString &name)
{
    for (int i=mWatchVars.size()-1;i>=0;i--) {
        PWatchVar var = mWatchVars[i];
        if (name == var->name) {
            this->beginResetModel();
            //this->beginRemoveRows(QModelIndex(),i,i);
            mWatchVars.removeAt(i);
            //this->endRemoveRows();
            this->endResetModel();
        }
    }
}

void WatchModel::removeWatchVar(int gdbIndex)
{
    for (int i=mWatchVars.size()-1;i>=0;i--) {
        PWatchVar var = mWatchVars[i];
        if (gdbIndex == var->gdbIndex) {
            this->beginResetModel();
            //this->beginRemoveRows(QModelIndex(),i,i);
            mWatchVars.removeAt(i);
            //this->endRemoveRows();
            this->endResetModel();
        }
    }
}

void WatchModel::removeWatchVar(const QModelIndex &index)
{
    int r=index.row();
    this->beginRemoveRows(QModelIndex(),r,r);
    mWatchVars.removeAt(r);
    this->endRemoveRows();
}

void WatchModel::clear()
{
    this->beginResetModel();
    mWatchVars.clear();
    this->endResetModel();
}

const QList<PWatchVar> &WatchModel::watchVars()
{
    return mWatchVars;
}

PWatchVar WatchModel::findWatchVar(const QString &name)
{
    for (PWatchVar var:mWatchVars) {
        if (name == var->name) {
            return var;
        }
    }
    return PWatchVar();
}

PWatchVar WatchModel::findWatchVar(int gdbIndex)
{
    for (PWatchVar var:mWatchVars) {
        if (gdbIndex == var->gdbIndex) {
            return var;
        }
    }
    return PWatchVar();
}

void WatchModel::beginUpdate()
{
    if (mUpdateCount == 0) {
        beginResetModel();
    }
    mUpdateCount++;
}

void WatchModel::endUpdate()
{
    mUpdateCount--;
    if (mUpdateCount == 0) {
        endResetModel();
    }
}

void WatchModel::notifyUpdated(PWatchVar var)
{
    if (!var)
        return;
    int row;
    if (var->parent==nullptr) {
        row = mWatchVars.indexOf(var);
    } else {
        row = var->parent->children.indexOf(var);
    }
    if (row<0)
        return;
    //qDebug()<<"dataChanged"<<row<<":"<<var->text;
    emit dataChanged(createIndex(row,0,var.get()),createIndex(row,0,var.get()));
}

QVariant WatchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role ==  Qt::DisplayRole) {
        switch(section) {
        case 0:
            return tr("Expression");
        case 1:
            return tr("Value");
        }
    }
    return QVariant();
}

RegisterModel::RegisterModel(QObject *parent):QAbstractTableModel(parent)
{

}

int RegisterModel::rowCount(const QModelIndex &parent) const
{
    return mRegisters.count();
}

int RegisterModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant RegisterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row()<0 || index.row() >= static_cast<int>(mRegisters.size()))
        return QVariant();
    PRegister reg = mRegisters[index.row()];
    if (!reg)
        return QVariant();
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return reg->name;
        case 1:
            return reg->hexValue;
        case 2:
            return reg->decValue;
        default:
            return QVariant();
        }
    default:
        return QVariant();
    }
}

QVariant RegisterModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role ==  Qt::DisplayRole) {
        switch(section) {
        case 0:
            return tr("Register");
        case 1:
            return tr("Value(Hex)");
        case 2:
            return tr("Value(Dec)");
        }
    }
    return QVariant();
}

void RegisterModel::update(const QList<PRegister> &regs)
{
    beginResetModel();
    mRegisters.clear();
    mRegisters.append(regs);
    endResetModel();
}


void RegisterModel::clear()
{
    beginResetModel();
    mRegisters.clear();
    endResetModel();
}
