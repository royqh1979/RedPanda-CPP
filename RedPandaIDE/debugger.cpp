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
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "widgets/signalmessagedialog.h"

Debugger::Debugger(QObject *parent) : QObject(parent)
{
    mBreakpointModel=new BreakpointModel(this);
    mBacktraceModel=new BacktraceModel(this);
    mWatchModel = new WatchModel(this);
    mRegisterModel = new RegisterModel(this);
    mExecuting = false;
    mReader = nullptr;
    mTarget = nullptr;
    mCommandChanged = false;
    mLeftPageIndexBackup = -1;

    connect(mWatchModel, &WatchModel::fetchChildren,
            this, &Debugger::fetchVarChildren);
}

bool Debugger::start(const QString& inferior)
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
    if (pSettings->debugger().useGDBServer()) {
        if (!isTextAllAscii(compilerSet->debugServer())) {
            mExecuting = false;
            QMessageBox::critical(pMainWindow,
                                  tr("GDB Server path error"),
                                  tr("GDB Server's path \"%1\" contains non-ascii characters.")
                                  .arg(compilerSet->debugServer())
                                  + "<br />"
                                  + tr("This prevents it from executing."));
            return false;
        }
        if (!fileExists(compilerSet->debugServer())) {
            mExecuting = false;
            QMessageBox::critical(pMainWindow,
                                  tr("GDB Server not exists"),
                                  tr("Can''t find gdb server in : \"%1\"").arg(compilerSet->debugServer()));
            return false;
        }

    }
    mWatchModel->resetAllVarInfos();
    if (pSettings->debugger().useGDBServer()) {
        mTarget = new DebugTarget(inferior,compilerSet->debugServer(),pSettings->debugger().GDBServerPort());
        connect(mTarget, &QThread::finished,[this](){
            if (mExecuting) {
                stop();
            }
            mTarget->deleteLater();
            mTarget = nullptr;
        });
        mTarget->start();
        mTarget->waitStart();
    }
    mReader = new DebugReader(this);
    mReader->setDebuggerPath(debuggerPath);
    connect(mReader, &QThread::finished,this,&Debugger::cleanUpReader);
    connect(mReader, &DebugReader::parseFinished,this,&Debugger::syncFinishedParsing,Qt::BlockingQueuedConnection);
    connect(mReader, &DebugReader::changeDebugConsoleLastLine,this,&Debugger::onChangeDebugConsoleLastline);
    connect(mReader, &DebugReader::cmdStarted,pMainWindow, &MainWindow::disableDebugActions);
    connect(mReader, &DebugReader::cmdFinished,pMainWindow, &MainWindow::enableDebugActions);
    connect(mReader, &DebugReader::inferiorStopped, pMainWindow, &MainWindow::enableDebugActions);

    connect(mReader, &DebugReader::breakpointInfoGetted, mBreakpointModel,
            &BreakpointModel::updateBreakpointNumber);
    connect(mReader, &DebugReader::localsUpdated, pMainWindow,
            &MainWindow::onLocalsReady);
    connect(mReader, &DebugReader::memoryUpdated,this,
            &Debugger::updateMemory);
    connect(mReader, &DebugReader::evalUpdated,this,
            &Debugger::updateEval);
    connect(mReader, &DebugReader::disassemblyUpdate,this,
            &Debugger::updateDisassembly);
    connect(mReader, &DebugReader::registerNamesUpdated, this,
            &Debugger::updateRegisterNames);
    connect(mReader, &DebugReader::registerValuesUpdated, this,
            &Debugger::updateRegisterValues);
    connect(mReader, &DebugReader::varCreated,mWatchModel,
            &WatchModel::updateVarInfo);
    connect(mReader, &DebugReader::prepareVarChildren,mWatchModel,
            &WatchModel::prepareVarChildren);
    connect(mReader, &DebugReader::addVarChild,mWatchModel,
            &WatchModel::addVarChild);
    connect(mReader, &DebugReader::varValueUpdated,mWatchModel,
            &WatchModel::updateVarValue);
    connect(mReader, &DebugReader::inferiorContinued,pMainWindow,
            &MainWindow::removeActiveBreakpoints);
    connect(mReader, &DebugReader::inferiorStopped,pMainWindow,
            &MainWindow::setActiveBreakpoint);
    connect(mReader, &DebugReader::inferiorStopped,this,
            &Debugger::refreshWatchVars);

    mReader->registerInferiorStoppedCommand("-stack-list-frames","");
    mReader->registerInferiorStoppedCommand("-stack-list-variables", "--all-values");
    mReader->start();
    mReader->waitStart();

    pMainWindow->updateAppTitle();

    //Application.HintHidePause := 5000;
    return true;
}
void Debugger::stop() {
    if (mExecuting) {
        if (mTarget) {
            mTarget->stopDebug();
            mTarget = nullptr;
        }
        mReader->stopDebug();
    }
}
void Debugger::cleanUpReader()
{
    if (mExecuting) {
        mExecuting = false;

        //stop debugger
        mReader->deleteLater();
        mReader=nullptr;

        if (pMainWindow->cpuDialog()!=nullptr) {
            pMainWindow->cpuDialog()->close();
        }

        // Free resources
        pMainWindow->removeActiveBreakpoints();

        pMainWindow->txtLocals()->clear();

        pMainWindow->updateAppTitle();

        pMainWindow->updateDebugEval("");

        mBacktraceModel->clear();

        mWatchModel->clearAllVarInfos();

        mBreakpointModel->invalidateAllBreakpointNumbers();

        pMainWindow->updateEditorActions();
    }
}

void Debugger::updateRegisterNames(const QStringList &registerNames)
{
    mRegisterModel->updateNames(registerNames);
}

void Debugger::updateRegisterValues(const QHash<int, QString> &values)
{
    mRegisterModel->updateValues(values);
}

RegisterModel *Debugger::registerModel() const
{
    return mRegisterModel;
}

WatchModel *Debugger::watchModel() const
{
    return mWatchModel;
}

void Debugger::sendCommand(const QString &command, const QString &params, DebugCommandSource source)
{
    if (mExecuting && mReader) {
        mReader->postCommand(command,params,source);
    }
}

bool Debugger::commandRunning()
{
    if (mExecuting && mReader) {
        return mReader->commandRunning();
    }
    return false;
}

bool Debugger::inferiorRunning()
{
    if (mExecuting && mReader) {
        return mReader->inferiorRunning();
    }
    return false;
}

void Debugger::interrupt()
{
    sendCommand("-exec-interrupt", "");
    QTimer::singleShot(1000,this, &Debugger::interruptRefresh);

}

void Debugger::addBreakpoint(int line, const Editor* editor)
{
    addBreakpoint(line,editor->filename());
}

void Debugger::addBreakpoint(int line, const QString &filename)
{
    PBreakpoint bp=std::make_shared<Breakpoint>();
    bp->number = -1;
    bp->line = line;
    bp->filename = filename;
    bp->condition = "";
    bp->enabled = true;
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
        sendCommand("-break-condition",
                    QString("%1").arg(breakpoint->line));
    } else {
        sendCommand("-break-condition",
                    QString("%1 %2").arg(breakpoint->line).arg(condition));
    }
}

void Debugger::sendAllBreakpointsToDebugger()
{
    for (PBreakpoint breakpoint:mBreakpointModel->breakpoints()) {
        sendBreakpointCommand(breakpoint);
    }
}

void Debugger::addWatchVar(const QString &expression)
{
    // Don't allow duplicates...
    PWatchVar oldVar = mWatchModel->findWatchVar(expression);
    if (oldVar)
        return;

    PWatchVar var = std::make_shared<WatchVar>();
    var->parent= nullptr;
    var->expression = expression;
    var->value = tr("Execute to evaluate");
    var->numChild = 0;
    var->hasMore = false;
    var->parent = nullptr;

    mWatchModel->addWatchVar(var);
    sendWatchCommand(var);
}

void Debugger::modifyWatchVarExpression(const QString &oldExpr, const QString &newExpr)
{
    // check if name already exists;
    PWatchVar var = mWatchModel->findWatchVar(newExpr);
    if (var)
        return;

    var = mWatchModel->findWatchVar(oldExpr);
    if (var) {
        if (mExecuting && !var->expression.isEmpty())
            sendRemoveWatchCommand(var);
        var->expression = newExpr;
        var->type.clear();
        var->value.clear();
        var->hasMore = false;
        var->numChild=0;
        var->name.clear();
        var->children.clear();

        if (mExecuting) {
            sendWatchCommand(var);
        }
    }
}

void Debugger::refreshWatchVars()
{
    if (mExecuting) {
        sendAllWatchVarsToDebugger();
        sendCommand("-var-update"," --all-values *");
    }
}

void Debugger::fetchVarChildren(const QString &varName)
{
    if (mExecuting) {
        sendCommand("-var-list-children",varName);
    }
}

void Debugger::interruptRefresh()
{
    sendCommand("noop","");
}

void Debugger::removeWatchVars(bool deleteparent)
{
    if (deleteparent) {
        mWatchModel->clear();
    } else {
        for(const PWatchVar& var:mWatchModel->watchVars()) {
            sendRemoveWatchCommand(var);
        }
        mWatchModel->clearAllVarInfos();
    }
}

void Debugger::removeWatchVar(const QModelIndex &index)
{
    PWatchVar var = mWatchModel->findWatchVar(index);
    if (!var)
        return;
    sendRemoveWatchCommand(var);
    mWatchModel->removeWatchVar(index);
}

void Debugger::sendAllWatchVarsToDebugger()
{
    for (PWatchVar var:mWatchModel->watchVars()) {
        if (var->name.isEmpty())
            sendWatchCommand(var);
    }
}

PWatchVar Debugger::findWatchVar(const QString &expression)
{
    return mWatchModel->findWatchVar(expression);
}

//void Debugger::notifyWatchVarUpdated(PWatchVar var)
//{
//    mWatchModel->notifyUpdated(var);
//}
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
    sendCommand("-var-create", var->expression);
}

void Debugger::sendRemoveWatchCommand(PWatchVar var)
{
    sendCommand("-var-delete",QString("%1").arg(var->name));
}

void Debugger::sendBreakpointCommand(PBreakpoint breakpoint)
{
    if (breakpoint && mExecuting) {
        // break "filename":linenum
        QString condition;
        if (!breakpoint->condition.isEmpty()) {
            condition = " -c " + breakpoint->condition;
        }
        QString filename = breakpoint->filename;
        filename.replace('\\','/');
        sendCommand("-break-insert",
                    QString("%1 --source \"%2\" --line %3")
                    .arg(condition,filename)
                    .arg(breakpoint->line));
    }
}

void Debugger::sendClearBreakpointCommand(int index)
{
    sendClearBreakpointCommand(mBreakpointModel->breakpoints()[index]);
}

void Debugger::sendClearBreakpointCommand(PBreakpoint breakpoint)
{
    // Debugger already running? Remove it from GDB
    if (breakpoint && breakpoint->number>=0 && mExecuting) {
        //clear "filename":linenum
        QString filename = breakpoint->filename;
        filename.replace('\\','/');
        sendCommand("-break-delete",
                QString("%1").arg(breakpoint->number));
    }
}

void Debugger::syncFinishedParsing()
{
    bool spawnedcpuform = false;

    // GDB determined that the source code is more recent than the executable. Ask the user if he wants to rebuild.
    if (mReader->receivedSFWarning()) {
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



    // show command output
    if (pSettings->debugger().enableDebugConsole() ) {
        if (pSettings->debugger().showDetailLog()) {
            for (const QString& line:mReader->fullOutput()) {
                pMainWindow->addDebugOutput(line);
            }
        } else {
            if (mReader->currentCmd() && mReader->currentCmd()->command == "disas") {

            } else {
                for (const QString& line:mReader->consoleOutput()) {
                    pMainWindow->addDebugOutput(line);
                }
                if (
                       (mReader->currentCmd()
                        && mReader->currentCmd()->source== DebugCommandSource::Console)
                        || !mReader->consoleOutput().isEmpty() ) {
                    pMainWindow->addDebugOutput("(gdb)");
                }
            }
        }
    }

    // The program to debug has stopped. Stop the debugger
    if (mReader->processExited()) {
        stop();
        return;
    }

    if (mReader->signalReceived()
            && mReader->signalName()!="SIGINT"
            && mReader->signalName()!="SIGTRAP") {
        SignalMessageDialog dialog(pMainWindow);
        dialog.setOpenCPUInfo(pSettings->debugger().openCPUInfoWhenSignaled());
        dialog.setMessage(
                    tr("Signal \"%1\" Received: ").arg(mReader->signalName())
                    + "<br />"
                    + mReader->signalMeaning());
        int result = dialog.exec();
        if (result == QDialog::Accepted && dialog.openCPUInfo()) {
            pMainWindow->showCPUInfoDialog();
        }
    }

    // CPU form updates itself when spawned, don't update twice!
    if ((mReader->updateCPUInfo() && !spawnedcpuform) && (pMainWindow->cpuDialog()!=nullptr)) {
        pMainWindow->cpuDialog()->updateInfo();
    }
}

void Debugger::updateMemory(const QStringList &value)
{
    emit memoryExamineReady(value);
}

void Debugger::updateEval(const QString &value)
{
    emit evalValueReady(value);
}

void Debugger::updateDisassembly(const QString& file, const QString& func, const QStringList &value)
{
    if (pMainWindow->cpuDialog()) {
        pMainWindow->cpuDialog()->setDisassembly(file,func,value);
    }
}

void Debugger::onChangeDebugConsoleLastline(const QString& text)
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
    mProcess = std::make_shared<QProcess>();
    mCmdRunning = false;
}

void DebugReader::postCommand(const QString &Command, const QString &Params,
                               DebugCommandSource Source)
{
    QMutexLocker locker(&mCmdQueueMutex);
    PDebugCommand pCmd = std::make_shared<DebugCommand>();
    pCmd->command = Command;
    pCmd->params = Params;
    pCmd->source = Source;
    mCmdQueue.enqueue(pCmd);
//    if (!mCmdRunning)
    //        runNextCmd();
}

void DebugReader::registerInferiorStoppedCommand(const QString &Command, const QString &Params)
{
    QMutexLocker locker(&mCmdQueueMutex);
    PDebugCommand pCmd = std::make_shared<DebugCommand>();
    pCmd->command = Command;
    pCmd->params = Params;
    pCmd->source = DebugCommandSource::Other;
    mInferiorStoppedHookCommands.append(pCmd);
}

void DebugReader::clearCmdQueue()
{
    QMutexLocker locker(&mCmdQueueMutex);
    mCmdQueue.clear();
}

void DebugReader::processConsoleOutput(const QByteArray& line)
{
    if (line.length()>3 && line.startsWith("~\"") && line.endsWith("\"")) {
        QByteArray s=line.mid(2,line.length()-3);
        QByteArray stringValue;
        const char *p=s.data();
        while (*p!=0) {
            if (*p=='\\' && *(p+1)!=0) {
                p++;
                switch (*p) {
                case '\'':
                    stringValue+=0x27;
                    p++;
                    break;
                case '"':
                    stringValue+=0x22;
                    p++;
                    break;
                case '?':
                    stringValue+=0x3f;
                    p++;
                    break;
                case '\\':
                    stringValue+=0x5c;
                    p++;
                    break;
                case 'a':
                    stringValue+=0x07;
                    p++;
                    break;
                case 'b':
                    stringValue+=0x08;
                    p++;
                    break;
                case 'f':
                    stringValue+=0x0c;
                    p++;
                    break;
                case 'n':
                    stringValue+=0x0a;
                    p++;
                    break;
                case 'r':
                    stringValue+=0x0d;
                    p++;
                    break;
                case 't':
                    stringValue+=0x09;
                    p++;
                    break;
                case 'v':
                    stringValue+=0x0b;
                    p++;
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                {
                    int i=0;
                    for (i=0;i<3;i++) {
                        if (*(p+i)<'0' || *(p+i)>'7')
                            break;
                    }
                    QByteArray numStr(p,i);
                    bool ok;
                    unsigned char ch = numStr.toInt(&ok,8);
                    stringValue+=ch;
                    p+=i;
                    break;
                }
                }
            } else {
                stringValue+=*p;
                p++;
            }
        }
        mConsoleOutput.append(QString::fromLocal8Bit(stringValue));
    }
}

void DebugReader::processResult(const QByteArray &result)
{
    GDBMIResultParser parser;
    GDBMIResultType resultType;
    GDBMIResultParser::ParseObject multiValues;
    if (!mCurrentCmd)
        return;
    bool parseOk = parser.parse(result, mCurrentCmd->command, resultType,multiValues);
    if (!parseOk)
        return;
    switch(resultType) {
    case GDBMIResultType::BreakpointTable:
    case GDBMIResultType::Frame:
    case GDBMIResultType::Locals:
        break;
    case GDBMIResultType::Breakpoint:
        handleBreakpoint(multiValues["bkpt"].object());
        return;
    case GDBMIResultType::FrameStack:
        handleStack(multiValues["stack"].array());
        return;
    case GDBMIResultType::LocalVariables:
        handleLocalVariables(multiValues["variables"].array());
        return;
    case GDBMIResultType::Evaluation:
        handleEvaluation(multiValues["value"].value());
        return;
    case GDBMIResultType::Memory:
        handleMemory(multiValues["memory"].array());
        return;
    case GDBMIResultType::RegisterNames:
        handleRegisterNames(multiValues["register-names"].array());
        return;
    case GDBMIResultType::RegisterValues:
        handleRegisterValue(multiValues["register-values"].array());
        return;
    case GDBMIResultType::CreateVar:
        handleCreateVar(multiValues);
        return;
    case GDBMIResultType::ListVarChildren:
        handleListVarChildren(multiValues);
        return;
    case GDBMIResultType::UpdateVarValue:
        handleUpdateVarValue(multiValues["changelist"].array());
        return;
    }

}

void DebugReader::processExecAsyncRecord(const QByteArray &line)
{
    QByteArray result;
    GDBMIResultParser::ParseObject multiValues;
    GDBMIResultParser parser;
    if (!parser.parseAsyncResult(line,result,multiValues))
        return;
    if (result == "running") {
        mInferiorRunning = true;
        mCurrentAddress=0;
        mCurrentFile.clear();
        mCurrentLine=-1;
        mCurrentFunc.clear();
        emit inferiorContinued();
        return;
    }
    if (result == "stopped") {
        mInferiorRunning = false;
        QByteArray reason = multiValues["reason"].value();
        if (reason == "exited") {
            //inferior exited, gdb should terminate too
            mProcessExited = true;
            return;
        }
        if (reason == "exited-normally") {
            //inferior exited, gdb should terminate too
            mProcessExited = true;
            return;
        }
        if (reason == "exited-signalled") {
            //inferior exited, gdb should terminate too
            mProcessExited = true;
            mSignalReceived = true;
            return;
        }
        mUpdateCPUInfo = true;
        GDBMIResultParser::ParseValue frame(multiValues["frame"]);
        if (frame.isValid()) {
            GDBMIResultParser::ParseObject frameObj = frame.object();
            mCurrentAddress = frameObj["addr"].hexValue();
            mCurrentLine = frameObj["line"].intValue();
            mCurrentFile = frameObj["fullname"].pathValue();
            mCurrentFunc = frameObj["func"].value();
        }
        if (reason == "signal-received") {
            mSignalReceived = true;
            mSignalName = multiValues["signal-name"].value();
            mSignalMeaning = multiValues["signal-meaning"].value();
        }
        runInferiorStoppedHook();
        if (mCurrentCmd && mCurrentCmd->source == DebugCommandSource::Console)
            emit inferiorStopped(mCurrentFile, mCurrentLine,false);
        else
            emit inferiorStopped(mCurrentFile, mCurrentLine,true);
    }
}

void DebugReader::processError(const QByteArray &errorLine)
{
    mConsoleOutput.append(QString::fromLocal8Bit(errorLine));
}

void DebugReader::processResultRecord(const QByteArray &line)
{
    if (line.startsWith("^exit")) {
        mProcessExited = true;
        return;
    }
    if (line.startsWith("^error")) {
        processError(line);
        return;
    }
    if (line.startsWith("^done")
            || line.startsWith("^running")) {
        int pos = line.indexOf(',');
        if (pos>=0) {
            QByteArray result = line.mid(pos+1);
            processResult(result);
        } else if (mCurrentCmd && !(mCurrentCmd->command.startsWith('-'))) {
            if (mCurrentCmd->command == "disas") {
                 QStringList disOutput = mConsoleOutput;
                if (disOutput.length()>=3) {
                    disOutput.pop_back();
                    disOutput.pop_front();
                    disOutput.pop_front();
                }
                emit disassemblyUpdate(mCurrentFile,mCurrentFunc, disOutput);
            }
        }
        return ;
    }
    if (line.startsWith("^connected")) {
        //TODO: connected to remote target
        return;
    }
}

void DebugReader::processDebugOutput(const QByteArray& debugOutput)
{
    // Only update once per update at most
    //WatchView.Items.BeginUpdate;

    emit parseStarted();

    mConsoleOutput.clear();
    mFullOutput.clear();

    mSignalReceived = false;
    mUpdateCPUInfo = false;
    mReceivedSFWarning = false;
    QList<QByteArray> lines = splitByteArrayToLines(debugOutput);

    for (int i=0;i<lines.count();i++) {
         QByteArray line = lines[i];
         if (pSettings->debugger().showDetailLog())
            mFullOutput.append(line);
         line = removeToken(line);
         if (line.isEmpty()) {
             continue;
         }
         switch (line[0]) {
         case '~': // console stream output
             processConsoleOutput(line);
             break;
         case '@': // target stream output
         case '&': // log stream output
             break;
         case '^': // result record
             processResultRecord(line);
             break;
         case '*': // exec async output
             processExecAsyncRecord(line);
             break;
         case '+': // status async output
         case '=': // notify async output
             break;
         }
    }
    emit parseFinished();
    mConsoleOutput.clear();
    mFullOutput.clear();
}

void DebugReader::runInferiorStoppedHook()
{
    foreach (const PDebugCommand& cmd, mInferiorStoppedHookCommands) {
        mCmdQueue.push_front(cmd);
    }
}

void DebugReader::runNextCmd()
{
    QMutexLocker locker(&mCmdQueueMutex);

    if (mCurrentCmd) {
        mCurrentCmd.reset();
        emit cmdFinished();
    }
    if (mCmdQueue.isEmpty())
        return;

    PDebugCommand pCmd = mCmdQueue.dequeue();
    mCmdRunning = true;
    mCurrentCmd = pCmd;    
    emit cmdStarted();

    QByteArray s;
    QByteArray params;
    s=pCmd->command.toLocal8Bit();
    if (!pCmd->params.isEmpty()) {
        params = pCmd->params.toLocal8Bit();
    }
    if (pCmd->command == "-var-create") {
        //hack for variable creation,to easy remember var expression
        params = " - @ "+params;
    } else if (pCmd->command == "-var-list-children") {
        //hack for list variable children,to easy remember var expression
        params = " --all-values \"" + params+'\"';
    }
    s+=" "+params;
    s+= "\n";
    if (mProcess->write(s)<0) {
        emit writeToDebugFailed();
    }

//  if devDebugger.ShowCommandLog or pCmd^.ShowInConsole then begin
    if (pSettings->debugger().enableDebugConsole() ) {
        //update debug console
        if (pSettings->debugger().showDetailLog()
                && pCmd->source != DebugCommandSource::Console) {
            emit changeDebugConsoleLastLine(pCmd->command + ' ' + params);
        }
    }
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

bool DebugReader::outputTerminated(const QByteArray &text)
{
    QStringList lines = textToLines(QString::fromUtf8(text));
    foreach (const QString& line,lines) {
        if (line.trimmed() == "(gdb)")
            return true;
    }
    return false;
}

void DebugReader::handleBreakpoint(const GDBMIResultParser::ParseObject& breakpoint)
{
    // gdb use system encoding for file path
    QString filename = breakpoint["fullname"].pathValue();
    int line = breakpoint["line"].intValue();
    int number = breakpoint["number"].intValue();
    emit breakpointInfoGetted(filename, line , number);
}

void DebugReader::handleStack(const QList<GDBMIResultParser::ParseValue> & stack)
{
    mDebugger->backtraceModel()->clear();
    foreach (const GDBMIResultParser::ParseValue& frameValue, stack) {
        GDBMIResultParser::ParseObject frameObject = frameValue.object();
        PTrace trace = std::make_shared<Trace>();
        trace->funcname = frameObject["func"].value();
        trace->filename = frameObject["fullname"].pathValue();
        trace->line = frameObject["line"].intValue();
        trace->level = frameObject["level"].intValue(0);
        trace->address = frameObject["addr"].value();
        mDebugger->backtraceModel()->addTrace(trace);
    }
}

void DebugReader::handleLocalVariables(const QList<GDBMIResultParser::ParseValue> &variables)
{
    QStringList locals;
    foreach (const GDBMIResultParser::ParseValue& varValue, variables) {
        GDBMIResultParser::ParseObject varObject = varValue.object();
        locals.append(QString("%1 = %2")
                            .arg(varObject["name"].value(),varObject["value"].value()));
    }
    emit localsUpdated(locals);
}

void DebugReader::handleEvaluation(const QString &value)
{
    emit evalUpdated(value);
}

void DebugReader::handleMemory(const QList<GDBMIResultParser::ParseValue> &rows)
{
    QStringList memory;
    foreach (const GDBMIResultParser::ParseValue& row, rows) {
        GDBMIResultParser::ParseObject rowObject = row.object();
        QList<GDBMIResultParser::ParseValue> data = rowObject["data"].array();
        QStringList values;
        foreach (const GDBMIResultParser::ParseValue& val, data) {
            values.append(val.value());
        }
        memory.append(QString("%1 %2")
                            .arg(rowObject["addr"].value(),values.join(" ")));
    }
    emit memoryUpdated(memory);
}

void DebugReader::handleRegisterNames(const QList<GDBMIResultParser::ParseValue> &names)
{
    QStringList nameList;
    foreach (const GDBMIResultParser::ParseValue& nameValue, names) {
        nameList.append(nameValue.value());
    }
    emit registerNamesUpdated(nameList);
}

void DebugReader::handleRegisterValue(const QList<GDBMIResultParser::ParseValue> &values)
{
    QHash<int,QString> result;
    foreach (const GDBMIResultParser::ParseValue& val, values) {
        GDBMIResultParser::ParseObject obj = val.object();
        int number = obj["number"].intValue();
        QString value = obj["value"].value();
        bool ok;
        long long intVal;
        intVal = value.toLongLong(&ok,10);
        if (ok) {
            value = QString("0x%1").arg(intVal,0,16);
        }
        result.insert(number,value);
    }
    emit registerValuesUpdated(result);
}

void DebugReader::handleCreateVar(const GDBMIResultParser::ParseObject &multiVars)
{
    if (!mCurrentCmd)
        return;
    QString expression = mCurrentCmd->params;
    QString name = multiVars["name"].value();
    int numChild = multiVars["numchild"].intValue(0);
    QString value = multiVars["value"].value();
    QString type = multiVars["type"].value();
    bool hasMore = multiVars["has_more"].value() != "0";
    emit varCreated(expression,name,numChild,value,type,hasMore);
}

void DebugReader::handleListVarChildren(const GDBMIResultParser::ParseObject &multiVars)
{
    if (!mCurrentCmd)
        return;
    QString parentName = mCurrentCmd->params;
    int parentNumChild = multiVars["numchild"].intValue(0);
    QList<GDBMIResultParser::ParseValue> children = multiVars["children"].array();
    bool hasMore = multiVars["has_more"].value()!="0";
    emit prepareVarChildren(parentName,parentNumChild,hasMore);
    foreach(const GDBMIResultParser::ParseValue& child, children) {
        GDBMIResultParser::ParseObject childObj = child.object();
        QString name = childObj["name"].value();
        QString exp = childObj["exp"].value();
        int numChild = childObj["numchild"].intValue(0);
        QString value = childObj["value"].value();
        QString type = childObj["type"].value();
        bool hasMore = childObj["has_more"].value() != "0";
        emit addVarChild(parentName,
                         name,
                         exp,
                         numChild,
                         value,
                         type,
                         hasMore);
    }
}

void DebugReader::handleUpdateVarValue(const QList<GDBMIResultParser::ParseValue> &changes)
{
    foreach (const GDBMIResultParser::ParseValue& value, changes) {
        GDBMIResultParser::ParseObject obj = value.object();
        QString name = obj["name"].value();
        QString val = obj["value"].value();
        QString inScope = obj["in_scope"].value();
        bool typeChanged = (obj["type_changed"].value()=="true");
        QString newType = obj["new_type"].value();
        int newNumChildren = obj["new_num_children"].intValue(-1);
        bool hasMore = (obj["has_more"].value() == "1");
        emit varValueUpdated(name,val,inScope,typeChanged,newType,newNumChildren,
                             hasMore);
    }
}

QByteArray DebugReader::removeToken(const QByteArray &line)
{
    int p=0;
    while (p<line.length()) {
        QChar ch=line[p];
        if (ch<'0' || ch>'9') {
            break;
        }
        p++;
    }
    if (p<line.length())
        return line.mid(p);
    return line;
}

const QString &DebugReader::signalMeaning() const
{
    return mSignalMeaning;
}

const QString &DebugReader::signalName() const
{
    return mSignalName;
}

bool DebugReader::inferiorRunning() const
{
    return mInferiorRunning;
}

const QStringList &DebugReader::fullOutput() const
{
    return mFullOutput;
}

bool DebugReader::receivedSFWarning() const
{
    return mReceivedSFWarning;
}

bool DebugReader::updateCPUInfo() const
{
    return mUpdateCPUInfo;
}

const PDebugCommand &DebugReader::currentCmd() const
{
    return mCurrentCmd;
}

const QStringList &DebugReader::consoleOutput() const
{
    return mConsoleOutput;
}

bool DebugReader::signalReceived() const
{
    return mSignalReceived;
}

bool DebugReader::processExited() const
{
    return mProcessExited;
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

void DebugReader::waitStart()
{
    mStartSemaphore.acquire(1);
}

void DebugReader::run()
{
    mStop = false;
    mInferiorRunning = false;
    mProcessExited = false;
    mErrorOccured = false;
    QString cmd = mDebuggerPath;
//    QString arguments = "--annotate=2";
    QString arguments = "--interpret=mi --silent";
    QString workingDir = QFileInfo(mDebuggerPath).path();

    mProcess = std::make_shared<QProcess>();
    auto action = finally([&]{
        mProcess.reset();
    });
    mProcess->setProgram(cmd);
    mProcess->setArguments(QProcess::splitCommand(arguments));
    mProcess->setProcessChannelMode(QProcess::MergedChannels);
    QString cmdDir = extractFileDir(cmd);
    if (!cmdDir.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString path = env.value("PATH");
        cmdDir.replace("/",QDir::separator());
        if (path.isEmpty()) {
            path = cmdDir;
        } else {
            path = cmdDir + PATH_SEPARATOR + path;
        }
        env.insert("PATH",path);
        mProcess->setProcessEnvironment(env);
    }
    mProcess->setWorkingDirectory(workingDir);

    connect(mProcess.get(), &QProcess::errorOccurred,
                    [&](){
                        mErrorOccured= true;
                    });
    QByteArray buffer;
    QByteArray readed;

    mProcess->start();
    mProcess->waitForStarted(5000);
    mStartSemaphore.release(1);
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
        if (mErrorOccured)
            break;
        readed = mProcess->readAll();
        buffer += readed;

        if (readed.endsWith("\n")&& outputTerminated(buffer)) {
            processDebugOutput(buffer);
            buffer.clear();
            mCmdRunning = false;
            runNextCmd();
        } else if (!mCmdRunning && readed.isEmpty()){
            runNextCmd();
        } else if (readed.isEmpty()){
            msleep(1);
        }
    }
    if (mErrorOccured) {
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
    beginResetModel();
    mList.clear();
    endResetModel();
}

void BreakpointModel::removeBreakpoint(int row)
{
    beginRemoveRows(QModelIndex(),row,row);
    mList.removeAt(row);
    endRemoveRows();
}

void BreakpointModel::invalidateAllBreakpointNumbers()
{
    foreach (PBreakpoint bp,mList) {
        bp->number = -1;
    }
    //emit dateChanged(createIndex(0,0),)
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

void BreakpointModel::save(const QString &filename)
{
    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QJsonArray array;
        foreach (const PBreakpoint& breakpoint, mList) {
            QJsonObject obj;
            obj["filename"]=breakpoint->filename;
            obj["line"]=breakpoint->line;
            obj["condition"]=breakpoint->condition;
            obj["enabled"]=breakpoint->enabled;
            array.append(obj);
        }
        QJsonDocument doc;
        doc.setArray(array);
        if (file.write(doc.toJson())<0) {
            throw FileError(tr("Save file '%1' failed.")
                            .arg(filename));
        }
    } else {
        throw FileError(tr("Can't open file '%1' for write.")
                        .arg(filename));
    }
}

void BreakpointModel::load(const QString &filename)
{
    clear();
    QFile file(filename);
    if (!file.exists())
        return;
    if (file.open(QFile::ReadOnly)) {
        QByteArray content = file.readAll();
        QJsonParseError error;
        QJsonDocument doc(QJsonDocument::fromJson(content,&error));
        if (error.error  != QJsonParseError::NoError) {
            throw FileError(tr("Error in json file '%1':%2 : %3")
                            .arg(filename)
                            .arg(error.offset)
                            .arg(error.errorString()));
        }
        QJsonArray array = doc.array();
        for  (int i=0;i<array.count();i++) {
            QJsonValue value = array[i];
            QJsonObject obj=value.toObject();
            PBreakpoint breakpoint = std::make_shared<Breakpoint>();
            breakpoint->filename = QFileInfo(obj["filename"].toString()).absoluteFilePath();
            breakpoint->line = obj["line"].toInt();
            breakpoint->condition = obj["condition"].toString();
            breakpoint->enabled = obj["enabled"].toBool();

            addBreakpoint(breakpoint);
        }
    } else {
        throw FileError(tr("Can't open file '%1' for read.")
                        .arg(filename));
    }
}

void BreakpointModel::updateBreakpointNumber(const QString& filename, int line, int number)
{
    foreach (PBreakpoint bp, mList) {
        if (bp->filename == filename && bp->line == line) {
            bp->number = number;
            return;
        }
    }
}

void BreakpointModel::onFileDeleteLines(const QString& filename, int startLine, int count)
{
    for (int i = mList.count()-1;i>=0;i--){
        PBreakpoint breakpoint = mList[i];
        if  (breakpoint->filename == filename
             && breakpoint->line>=startLine) {
            if (breakpoint->line >= startLine+count) {
                breakpoint->line -= count;
                emit dataChanged(createIndex(i,0),createIndex(i,2));
            } else {
                removeBreakpoint(i);
            }
        }
    }
}

void BreakpointModel::onFileInsertLines(const QString& filename, int startLine, int count)
{
    for (int i = mList.count()-1;i>=0;i--){
        PBreakpoint breakpoint = mList[i];
        if  (breakpoint->filename == filename
             && breakpoint->line>=startLine) {
            breakpoint->line+=count;
            emit dataChanged(createIndex(i,0),createIndex(i,2));
        }
    }
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
    beginResetModel();
    mList.clear();
    endResetModel();
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
        switch(index.column()) {
        case 0:
            return item->expression;
        case 1:
            return item->type;
        case 2:
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
    return 3;
}

void WatchModel::addWatchVar(PWatchVar watchVar)
{
    for (PWatchVar var:mWatchVars) {
        if (watchVar->expression == var->expression) {
            return;
        }
    }
    beginInsertRows(QModelIndex(),mWatchVars.count(),mWatchVars.count());
    mWatchVars.append(watchVar);
    endInsertRows();
}

void WatchModel::removeWatchVar(const QString &express)
{
    for (int i=mWatchVars.size()-1;i>=0;i--) {
        PWatchVar var = mWatchVars[i];
        if (express == var->expression) {
            this->beginResetModel();
            //this->beginRemoveRows(QModelIndex(),i,i);
            if (mVarIndex.contains(var->name))
                mVarIndex.remove(var->name);
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
    PWatchVar var = mWatchVars[r];
    if (mVarIndex.contains(var->name))
        mVarIndex.remove(var->name);
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

PWatchVar WatchModel::findWatchVar(const QModelIndex &index)
{
    if (!index.isValid())
        return PWatchVar();
    int r=index.row();
    return mWatchVars[r];
}

PWatchVar WatchModel::findWatchVar(const QString &expr)
{
    for (PWatchVar var:mWatchVars) {
        if (expr == var->expression) {
            return var;
        }
    }
    return PWatchVar();
}

void WatchModel::resetAllVarInfos()
{
    beginResetModel();
    foreach (PWatchVar var, mWatchVars) {
        var->name.clear();
        var->value = tr("Not Valid");
        var->numChild = 0;
        var->hasMore = false;
        var->type.clear();
        var->children.clear();
    }
    mVarIndex.clear();
    endResetModel();
}

void WatchModel::updateVarInfo(const QString &expression, const QString &name, int numChild, const QString &value, const QString &type, bool hasMore)
{
    PWatchVar var = findWatchVar(expression);
    if (!var)
        return;
    var->name = name;
    var->value = value;
    var->numChild = numChild;
    var->hasMore = hasMore;
    var->type = type;
    mVarIndex.insert(name,var);
    QModelIndex idx = index(var);
    if (!idx.isValid())
        return;
    emit dataChanged(idx,createIndex(idx.row(),2,var.get()));
}

void WatchModel::prepareVarChildren(const QString &parentName, int numChild, bool hasMore)
{
    PWatchVar var = mVarIndex.value(parentName,PWatchVar());
    if (var) {
        var->numChild = numChild;
        var->hasMore = hasMore;
        if (var->children.count()>0) {
            beginRemoveRows(index(var),0,var->children.count()-1);
            var->children.clear();
            endRemoveRows();
        }
    }
}

void WatchModel::addVarChild(const QString &parentName, const QString &name,
                             const QString &exp, int numChild, const QString &value,
                             const QString &type, bool hasMore)
{
    PWatchVar var = mVarIndex.value(parentName,PWatchVar());
    if (!var)
        return;
    beginInsertRows(index(var),var->children.count(),var->children.count());
    PWatchVar child = std::make_shared<WatchVar>();
    child->name = name;
    child->expression = exp;
    child->numChild = numChild;
    child->value = value;
    child->type = type;
    child->hasMore = hasMore;
    child->parent = var.get();
    var->children.append(child);
    endInsertRows();
    mVarIndex.insert(name,child);
}

void WatchModel::updateVarValue(const QString &name, const QString &val, const QString &inScope, bool typeChanged, const QString &newType, int newNumChildren, bool hasMore)
{
    PWatchVar var = mVarIndex.value(name,PWatchVar());
    if (!var)
        return;
    if (inScope == "true") {
        var->value = val;
    } else{
        var->value = tr("Not Valid");
    }
    if (typeChanged) {
        var->type = newType;
    }
    QModelIndex idx = index(var);
    bool oldHasMore = var->hasMore;
    var->hasMore = hasMore;
    if (newNumChildren>=0
            && var->numChild!=newNumChildren) {
        var->numChild = newNumChildren;
        fetchMore(idx);
    } else  if (!oldHasMore && hasMore) {
        fetchMore(idx);
    }
    emit dataChanged(idx,createIndex(idx.row(),2,var.get()));
}

void WatchModel::clearAllVarInfos()
{
    beginResetModel();
    foreach (PWatchVar var, mWatchVars) {
        var->name.clear();
        var->value = tr("Execute to evaluate");
        var->numChild = 0;
        var->hasMore = false;
        var->type.clear();
        var->children.clear();
    }
    mVarIndex.clear();
    endResetModel();
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

void WatchModel::save(const QString &filename)
{
    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        QJsonArray array;
        foreach (const PWatchVar& watchVar, mWatchVars) {
            QJsonObject obj;
            obj["expression"]=watchVar->expression;
            array.append(obj);
        }
        QJsonDocument doc;
        doc.setArray(array);
        if (file.write(doc.toJson())<0) {
            throw FileError(tr("Save file '%1' failed.")
                            .arg(filename));
        }
    } else {
        throw FileError(tr("Can't open file '%1' for write.")
                        .arg(filename));
    }
}

void WatchModel::load(const QString &filename)
{
    clear();
    QFile file(filename);
    if (!file.exists())
        return;
    if (file.open(QFile::ReadOnly)) {
        QByteArray content = file.readAll();
        QJsonParseError error;
        QJsonDocument doc(QJsonDocument::fromJson(content,&error));
        if (error.error  != QJsonParseError::NoError) {
            throw FileError(tr("Error in json file '%1':%2 : %3")
                            .arg(filename)
                            .arg(error.offset)
                            .arg(error.errorString()));
        }
        QJsonArray array = doc.array();
        for  (int i=0;i<array.count();i++) {
            QJsonValue value = array[i];
            QJsonObject obj=value.toObject();
            PWatchVar var = std::make_shared<WatchVar>();
            var->parent= nullptr;
            var->expression = obj["expression"].toString();
            var->value = tr("Execute to evaluate");
            var->numChild = 0;
            var->hasMore=false;
            var->parent = nullptr;

            addWatchVar(var);
        }
    } else {
        throw FileError(tr("Can't open file '%1' for read.")
                        .arg(filename));
    }
}

QModelIndex WatchModel::index(PWatchVar var) const
{
    if (!var)
        return QModelIndex();
    return index(var.get());
}

QModelIndex WatchModel::index(WatchVar* pVar) const {
    if (pVar==nullptr)
        return QModelIndex();
    if (pVar->parent) {
        int row=-1;
        for (int i=0;i<pVar->parent->children.count();i++) {
            if (pVar->parent->children[i].get() == pVar) {
                row = i;
                break;
            }
        }
        if (row<0)
            return QModelIndex();
        return createIndex(row,0,pVar);
    } else {
        int row=-1;
        for (int i=0;i<mWatchVars.count();i++) {
            if (mWatchVars[i].get() == pVar) {
                row = i;
                break;
            }
        }
        if (row<0)
            return QModelIndex();
        return createIndex(row,0,pVar);
    }
}


QVariant WatchModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role ==  Qt::DisplayRole) {
        switch(section) {
        case 0:
            return tr("Expression");
        case 1:
            return tr("Type");
        case 2:
            return tr("Value");
        }
    }
    return QVariant();
}

void WatchModel::fetchMore(const QModelIndex &parent)
{
    if (!parent.isValid()) {
        return;
    }
    WatchVar* item = static_cast<WatchVar*>(parent.internalPointer());
    item->hasMore = false;
    item->numChild = item->children.count();
    emit fetchChildren(item->name);
}

bool WatchModel::canFetchMore(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return false;
    }
    WatchVar* item = static_cast<WatchVar*>(parent.internalPointer());
    return item->numChild>item->children.count() || item->hasMore;
}

bool WatchModel::hasChildren(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return true;
    }
    WatchVar* item = static_cast<WatchVar*>(parent.internalPointer());
    return item->numChild>0;
}

RegisterModel::RegisterModel(QObject *parent):QAbstractTableModel(parent)
{

}

int RegisterModel::rowCount(const QModelIndex &) const
{
    return mRegisterNames.count();
}

int RegisterModel::columnCount(const QModelIndex &) const
{
    return 2;
}

QVariant RegisterModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row()<0 || index.row() >= static_cast<int>(mRegisterNames.size()))
        return QVariant();
    switch (role) {
    case Qt::DisplayRole:
        switch (index.column()) {
        case 0:
            return mRegisterNames[index.row()];
        case 1:
            return mRegisterValues.value(index.row(),"");
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
            return tr("Value");
        }
    }
    return QVariant();
}

void RegisterModel::updateNames(const QStringList &regNames)
{
    beginResetModel();
    mRegisterNames = regNames;
    endResetModel();
}

void RegisterModel::updateValues(const QHash<int, QString> registerValues)
{
    mRegisterValues= registerValues;
    emit dataChanged(createIndex(0,1),
                     createIndex(mRegisterNames.count()-1,1));
}


void RegisterModel::clear()
{
    beginResetModel();
    mRegisterNames.clear();
    mRegisterValues.clear();
    endResetModel();
}

DebugTarget::DebugTarget(
        const QString &inferior,
        const QString &GDBServer,
        int port,
        QObject *parent):
    QThread(parent),
    mInferior(inferior),
    mGDBServer(GDBServer),
    mPort(port),
    mStop(false),
    mStartSemaphore(0),
    mErrorOccured(false)
{
    mProcess = nullptr;
}

void DebugTarget::stopDebug()
{
    mStop = true;
}

void DebugTarget::waitStart()
{
    mStartSemaphore.acquire(1);
}

void DebugTarget::run()
{
    mStop = false;
    mErrorOccured = false;

    //find first available port
    QString cmd;
    QString arguments;
#ifdef Q_OS_WIN
    cmd= mGDBServer;
    arguments = QString(" localhost:%1 \"%2\"").arg(mPort).arg(mInferior);
#else
    if  (programHasConsole(mInferior)) {
        cmd= pSettings->environment().terminalPath();
        arguments = QString(" -e \"%1\" localhost:%2 \"%3\"").arg(mGDBServer).arg(mPort).arg(mInferior);
    } else {
        cmd= mGDBServer;
        arguments = QString(" localhost:%1 \"%2\"").arg(mPort).arg(mInferior);
    }
#endif
    QString workingDir = QFileInfo(mInferior).path();

    mProcess = std::make_shared<QProcess>();
    auto action = finally([&]{
        mProcess.reset();
    });
    mProcess->setProgram(cmd);
    mProcess->setArguments(QProcess::splitCommand(arguments));
    mProcess->setProcessChannelMode(QProcess::MergedChannels);
    QString cmdDir = extractFileDir(cmd);
    if (!cmdDir.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString path = env.value("PATH");
        cmdDir.replace("/",QDir::separator());
        if (path.isEmpty()) {
            path = cmdDir;
        } else {
            path = cmdDir + PATH_SEPARATOR + path;
        }
        env.insert("PATH",path);
        mProcess->setProcessEnvironment(env);
    }
    mProcess->setWorkingDirectory(workingDir);

#ifdef Q_OS_WIN
    mProcess->setCreateProcessArgumentsModifier([this](QProcess::CreateProcessArguments * args){
        if (programHasConsole(mInferior)) {
            args->flags |=  CREATE_NEW_CONSOLE;
            args->flags &= ~CREATE_NO_WINDOW;
        }
        args->startupInfo -> dwFlags &= ~STARTF_USESTDHANDLES;
    });
#endif

    connect(mProcess.get(), &QProcess::errorOccurred,
                    [&](){
                        mErrorOccured= true;
                    });
    mProcess->start();
    mProcess->waitForStarted(5000);
    mStartSemaphore.release(1);
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
        if (mErrorOccured)
            break;
        msleep(1);
    }
    if (mErrorOccured) {
        emit processError(mProcess->error());
    }
}
