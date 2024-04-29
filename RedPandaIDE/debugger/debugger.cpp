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
#include "gdbmidebugger.h"
#include "utils.h"
#include "utils/parsearg.h"
#include "mainwindow.h"
#include "editor.h"
#include "settings.h"
#include "widgets/cpudialog.h"
#include "systemconsts.h"
#include "editorlist.h"
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
#include <QApplication>
#include <QRegularExpression>

Debugger::Debugger(QObject *parent) : QObject(parent),
    mForceUTF8(false),
    mDebuggerType(DebuggerType::GDB),
    mLastLoadtime(0),
    mProjectLastLoadtime(0)
{
    //models deleted in the destructor
    mBreakpointModel= std::make_shared<BreakpointModel>(this);
    mBacktraceModel = std::make_shared<BacktraceModel>(this);
    mWatchModel = std::make_shared<WatchModel>(this);
    mRegisterModel = std::make_shared<RegisterModel>(this);
    mMemoryModel = std::make_shared<MemoryModel>(16,this);

    connect(mMemoryModel.get(),&MemoryModel::setMemoryData,
            this, &Debugger::setMemoryData);
    connect(mWatchModel.get(), &WatchModel::setWatchVarValue,
            this, &Debugger::setWatchVarValue);
    mExecuting = false;
    mClient = nullptr;
    mTarget = nullptr;
    mCommandChanged = false;
    mLeftPageIndexBackup = -1;

    connect(mWatchModel.get(), &WatchModel::fetchChildren,
            this, &Debugger::fetchVarChildren);

    setIsForProject(false);
}

Debugger::~Debugger()
{
//    delete mBreakpointModel;
//    delete mBacktraceModel;
//    delete mWatchModel;
//    delete mRegisterModel;
//    delete mMemoryModel;
}

bool Debugger::startClient(int compilerSetIndex,
                           const QString& inferior,
                           bool inferiorHasSymbols,
                           bool inferiorHasBreakpoints,
                           const QStringList& binDirs,
                           const QString& sourceFile)
{
    mCurrentSourceFile = sourceFile;
    Settings::PCompilerSet compilerSet = pSettings->compilerSets().getSet(compilerSetIndex);
    if (!compilerSet) {
        compilerSet = pSettings->compilerSets().defaultSet();
    }
    if (!compilerSet) {
        QMessageBox::critical(pMainWindow,
                              tr("No compiler set"),
                              tr("No compiler set is configured.")+tr("Can't start debugging."));
        return false;
    }
    setForceUTF8(compilerSet->forceUTF8());
    setDebugInfosUsingUTF8(compilerSet->isDebugInfoUsingUTF8());
    if (compilerSet->debugger().endsWith(LLDB_MI_PROGRAM))
        setDebuggerType(DebuggerType::LLDB_MI);
    else
        setDebuggerType(DebuggerType::GDB);
    // force to lldb-server if using lldb-mi, which creates new console but does not bind inferior’s stdio to the new console on Windows.
    setUseDebugServer(pSettings->debugger().useGDBServer() || mDebuggerType == DebuggerType::LLDB_MI);
    mExecuting = true;
    QString debuggerPath = compilerSet->debugger();
    //QFile debuggerProgram(debuggerPath);
//    if (!isTextAllAscii(debuggerPath)) {
//        mExecuting = false;
//        QMessageBox::critical(pMainWindow,
//                              tr("Debugger path error"),
//                              tr("Debugger's path \"%1\" contains non-ascii characters.")
//                              .arg(debuggerPath)
//                              + "<br />"
//                              + tr("This prevents it from executing."));
//        return false;
//    }
    if (!fileExists(debuggerPath)) {
        mExecuting = false;
        QMessageBox::critical(pMainWindow,
                              tr("Debugger not exists"),
                              tr("Can''t find debugger (gdb) in : \"%1\"").arg(debuggerPath)
                              +"<br />"
                              +tr("Please check the \"program\" page of compiler settings."));
        return false;
    }
    if (useDebugServer()) {
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
    mMemoryModel->reset();
    mWatchModel->resetAllVarInfos();
    if (useDebugServer()) {
        //deleted when thread finished
        QStringList params;
        if (pSettings->executor().useParams())
            params = parseArgumentsWithoutVariables(pSettings->executor().params());
        mTarget = new DebugTarget(inferior,compilerSet->debugServer(),pSettings->debugger().GDBServerPort(),params);
        if (pSettings->executor().redirectInput())
            mTarget->setInputFile(pSettings->executor().inputFilename());
        connect(mTarget, &QThread::finished,[this](){
            if (mExecuting) {
                stop();
            }
            mTarget->deleteLater();
            mTarget = nullptr;
        });
        mTarget->addBinDirs(binDirs);
        mTarget->addBinDir(pSettings->dirs().appDir());
        mTarget->start();
        mTarget->waitStart();
    }
    //delete when thread finished
    mClient = new GDBMIDebuggerClient(this, debuggerType());
    mClient->addBinDirs(binDirs);
    mClient->addBinDir(pSettings->dirs().appDir());
    mClient->setDebuggerPath(debuggerPath);
    connect(mClient, &QThread::finished,this,&Debugger::cleanUpReader);
    connect(mClient, &QThread::finished,mMemoryModel.get(),&MemoryModel::reset);
    connect(mClient, &DebuggerClient::parseFinished,this,&Debugger::syncFinishedParsing,Qt::BlockingQueuedConnection);
    connect(mClient, &DebuggerClient::changeDebugConsoleLastLine,this,&Debugger::onChangeDebugConsoleLastline);
    connect(mClient, &DebuggerClient::cmdStarted,pMainWindow, &MainWindow::disableDebugActions);
    connect(mClient, &DebuggerClient::cmdFinished,pMainWindow, &MainWindow::enableDebugActions);
    connect(mClient, &DebuggerClient::inferiorStopped, pMainWindow, &MainWindow::enableDebugActions);

    connect(mClient, &DebuggerClient::breakpointInfoGetted, mBreakpointModel.get(),
            &BreakpointModel::updateBreakpointNumber);
    connect(mClient, &DebuggerClient::localsUpdated, pMainWindow,
            &MainWindow::onLocalsReady);
    connect(mClient, &DebuggerClient::memoryUpdated,this,
            &Debugger::updateMemory);
    connect(mClient, &DebuggerClient::evalUpdated,this,
            &Debugger::updateEval);
    connect(mClient, &DebuggerClient::disassemblyUpdate,this,
            &Debugger::updateDisassembly);
    connect(mClient, &DebuggerClient::registerNamesUpdated, this,
            &Debugger::updateRegisterNames);
    connect(mClient, &DebuggerClient::registerValuesUpdated, this,
            &Debugger::updateRegisterValues);
    connect(mClient, &DebuggerClient::varCreated,mWatchModel.get(),
            &WatchModel::updateVarInfo);
    connect(mClient, &DebuggerClient::prepareVarChildren,mWatchModel.get(),
            &WatchModel::prepareVarChildren);
    connect(mClient, &DebuggerClient::addVarChild,mWatchModel.get(),
            &WatchModel::addVarChild);
    connect(mClient, &DebuggerClient::varValueUpdated,mWatchModel.get(),
            &WatchModel::updateVarValue);
    connect(mClient, &DebuggerClient::varsValueUpdated,mWatchModel.get(),
            &WatchModel::updateAllHasMoreVars);
    connect(mClient, &DebuggerClient::inferiorContinued,pMainWindow,
            &MainWindow::removeActiveBreakpoints);
    connect(mClient, &DebuggerClient::inferiorStopped,pMainWindow,
            &MainWindow::setActiveBreakpoint);
    connect(mClient, &DebuggerClient::watchpointHitted,pMainWindow,
            &MainWindow::onWatchpointHitted);
    connect(mClient, &DebuggerClient::errorNoSymbolTable,pMainWindow,
            &MainWindow::stopDebugForNoSymbolTable);
    connect(mClient, &DebuggerClient::inferiorStopped,this,
            &Debugger::refreshAll);

    mClient->start();
    mClient->waitStart();

    mClient->initialize(inferior, inferiorHasSymbols);
    includeOrSkipDirsInSymbolSearch(compilerSet->libDirs(), pSettings->debugger().skipCustomLibraries());
    includeOrSkipDirsInSymbolSearch(compilerSet->CIncludeDirs(), pSettings->debugger().skipCustomLibraries());
    includeOrSkipDirsInSymbolSearch(compilerSet->CppIncludeDirs(), pSettings->debugger().skipCustomLibraries());

    //gcc system libraries is auto loaded by gdb
    if (pSettings->debugger().skipSystemLibraries()) {
        includeOrSkipDirsInSymbolSearch(compilerSet->defaultCIncludeDirs(),true);
        includeOrSkipDirsInSymbolSearch(compilerSet->defaultCIncludeDirs(),true);
        includeOrSkipDirsInSymbolSearch(compilerSet->defaultCppIncludeDirs(),true);
    }

    sendAllBreakpointsToDebugger();
    pMainWindow->updateAppTitle();
    mInferiorHasBreakpoints = inferiorHasBreakpoints;
    return true;
}

void Debugger::runInferior()
{
    if (mClient)
        mClient->runInferior(mInferiorHasBreakpoints);
}

void Debugger::stop() {
    if (mExecuting) {
        if (mTarget) {
            mTarget->stopDebug();
            mTarget = nullptr;
        }
        mClient->stopDebug();
    }
    mCurrentSourceFile="";
}
void Debugger::cleanUpReader()
{
    if (mExecuting) {
        mExecuting = false;

        //stop debugger
        mClient->deleteLater();
        mClient=nullptr;

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

void Debugger::refreshAll()
{
    refreshWatchVars();
    if (mClient)
        mClient->refreshStackVariables();
    if (memoryModel()->startAddress()>0
            && mClient)
        mClient->readMemory(
                    QString("%1").arg(memoryModel()->startAddress()),
                    pSettings->debugger().memoryViewRows(),
                    pSettings->debugger().memoryViewColumns()
                    );
}

std::shared_ptr<RegisterModel> Debugger::registerModel() const
{
    return mRegisterModel;
}

std::shared_ptr<WatchModel> Debugger::watchModel() const
{
    return mWatchModel;
}

bool Debugger::commandRunning()
{
    if (mClient) {
        return mClient->commandRunning();
    }
    return false;
}

bool Debugger::inferiorRunning()
{
    if (mClient) {
        return mClient->inferiorRunning();
    }
    return false;
}

void Debugger::interrupt()
{
    if (mClient)
        mClient->interrupt();
}

void Debugger::stepOver()
{
    if (mClient)
        mClient->stepOver();
}

void Debugger::stepInto()
{
    if (mClient)
        mClient->stepInto();
}

void Debugger::stepOut()
{
    if (mClient)
        mClient->stepOut();
}

void Debugger::runTo(const QString &filename, int line)
{
    if (mClient)
        mClient->runTo(filename, line);
}

void Debugger::resume()
{
    if (mClient)
        mClient->resume();
}

void Debugger::stepOverInstruction()
{
    if (mClient)
        mClient->stepOverInstruction();
}

void Debugger::stepIntoInstruction()
{
    if (mClient)
        mClient->stepIntoInstruction();
}

void Debugger::runClientCommand(const QString &command, const QString &params, DebugCommandSource source)
{
    if (!mClient)
        return;
    if (mClient->clientType()!=DebuggerType::GDB
            && mClient->clientType()!=DebuggerType::LLDB_MI)
        return;
    GDBMIDebuggerClient* gdbmiClient = dynamic_cast<GDBMIDebuggerClient*>(mClient);
    gdbmiClient->postCommand(command, params, source);
}

bool Debugger::isForProject() const
{
    return mBreakpointModel->isForProject();
}

void Debugger::setIsForProject(bool newIsForProject)
{
    if (!executing()) {
        mBreakpointModel->setIsForProject(newIsForProject);
        mWatchModel->setIsForProject(newIsForProject);
    }
}

void Debugger::clearForProject()
{
    mBreakpointModel->clear(true);
    mWatchModel->clear(true);
}

void Debugger::addBreakpoint(int line, const Editor* editor)
{
    addBreakpoint(line,editor->filename(), editor->inProject());
}

void Debugger::addBreakpoint(int line, const QString &filename, bool forProject)
{
    PBreakpoint bp=std::make_shared<Breakpoint>();
    bp->number = -1;
    bp->line = line;
    bp->filename = filename;
    bp->condition = "";
    bp->enabled = true;
    bp->breakpointType = BreakpointType::Breakpoint;
    bp->timestamp = QDateTime::currentMSecsSinceEpoch();
    mBreakpointModel->addBreakpoint(bp,forProject);
    if (mExecuting) {
        if (forProject && mBreakpointModel->isForProject()) {
            sendBreakpointCommand(bp);
        } else if (filename == mCurrentSourceFile) {
            sendBreakpointCommand(bp);
        }
    }
}

void Debugger::deleteBreakpoints(const QString &filename, bool forProject)
{
    const QList<PBreakpoint>& list=mBreakpointModel->breakpoints(forProject);
    for (int i=list.size()-1;i>=0;i--) {
        PBreakpoint bp = list[i];
        if (bp->filename == filename) {
            mBreakpointModel->removeBreakpoint(i,forProject);
        }
    }
}

void Debugger::deleteBreakpoints(const Editor *editor)
{
    deleteBreakpoints(editor->filename(),editor->inProject());
}

void Debugger::deleteBreakpoints(bool forProject)
{
    mBreakpointModel->clear(forProject);
//    for (int i=mBreakpointModel->breakpoints().size()-1;i>=0;i--) {
//        removeBreakpoint(i);
    //    }
}

void Debugger::deleteInvalidProjectBreakpoints(const QSet<QString> unitFiles)
{
    for(int i=mBreakpointModel->breakpoints(true).count()-1;i>=0;i--) {
        const PBreakpoint& bp=mBreakpointModel->breakpoint(i,true);
        if (!unitFiles.contains(bp->filename))
            mBreakpointModel->removeBreakpoint(i, true);
    }
}

void Debugger::removeBreakpoint(int line, const Editor *editor)
{
    removeBreakpoint(line,editor->filename(),editor->inProject());
}

void Debugger::removeBreakpoint(int line, const QString &filename, bool forProject)
{
    const QList<PBreakpoint>& breakpoints=mBreakpointModel->breakpoints(forProject);
    for (int i=breakpoints.size()-1;i>=0;i--) {
        PBreakpoint bp = breakpoints[i];
        if (bp->filename == filename && bp->line == line) {
            removeBreakpoint(i, forProject);
        }
    }
}

void Debugger::removeBreakpoint(int index, bool forProject)
{
    sendClearBreakpointCommand(index, forProject);
    mBreakpointModel->removeBreakpoint(index, forProject);
}

PBreakpoint Debugger::breakpointAt(int line, const QString& filename, int *index , bool forProject)
{
    const QList<PBreakpoint>& breakpoints=mBreakpointModel->breakpoints(forProject);
    for (*index=0;*index<breakpoints.count();(*index)++){
        PBreakpoint breakpoint = breakpoints[*index];
        if (breakpoint->line == line
                && breakpoint->filename == filename)
            return breakpoint;
    }
    *index=-1;
    return PBreakpoint();
}

PBreakpoint Debugger::breakpointAt(int line, const Editor *editor, int *index)
{
    return breakpointAt(line,editor->filename(),index, editor->inProject());
}

void Debugger::setBreakPointCondition(int index, const QString &condition, bool forProject)
{
    PBreakpoint breakpoint=mBreakpointModel->setBreakPointCondition(index,condition, forProject);
    if (mClient)
        mClient->setBreakpointCondition(breakpoint);
}

void Debugger::sendAllBreakpointsToDebugger()
{
    for (PBreakpoint breakpoint:mBreakpointModel->breakpoints(mBreakpointModel->isForProject())) {
        if (mBreakpointModel->isForProject()) {
            sendBreakpointCommand(breakpoint);
        } else if (breakpoint->filename == mCurrentSourceFile) {
            sendBreakpointCommand(breakpoint);
        }
    }
}

void Debugger::saveForNonproject(const QString &filename)
{
    save(filename,QString());
}

void Debugger::saveForProject(const QString &filename, const QString &projectFolder)
{
    save(filename,projectFolder);
}

void Debugger::loadForNonproject(const QString &filename)
{
    bool forProject = false;
    mLastLoadtime = 0;
    PDebugConfig pConfig = load(filename, forProject);
    if (pConfig->timestamp>0) {
        mBreakpointModel->setBreakpoints(pConfig->breakpoints,forProject);
        mWatchModel->setWatchVars(pConfig->watchVars,forProject);
    }
}

void Debugger::loadForProject(const QString &filename, const QString &projectFolder)
{
    bool forProject = true;
    mProjectLastLoadtime = 0;
    PDebugConfig pConfig = load(filename, forProject);
    if (pConfig->timestamp>0) {
        foreach (const PBreakpoint& breakpoint, pConfig->breakpoints) {
            breakpoint->filename = generateAbsolutePath(projectFolder,breakpoint->filename);
        }
        mBreakpointModel->setBreakpoints(pConfig->breakpoints,forProject);
        mWatchModel->setWatchVars(pConfig->watchVars,forProject);
    }
}

void Debugger::addWatchpoint(const QString &expression)
{
    QString s=expression.trimmed();
    if (mClient) {
        mClient->addWatchpoint(expression);
    }
}

void Debugger::addWatchVar(const QString &expression)
{
    // Don't allow duplicates...
    PWatchVar oldVar = mWatchModel->findWatchVar(expression);
    if (oldVar)
        return;

    PWatchVar var = std::make_shared<WatchVar>();
    var->parent= PWatchVar();
    var->expression = expression;
    var->value = tr("Execute to evaluate");
    var->numChild = 0;
    var->hasMore = false;
    var->timestamp = QDateTime::currentMSecsSinceEpoch();

    addWatchVar(var,isForProject());
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

        sendWatchCommand(var);
    }
}

void Debugger::refreshWatchVars()
{
    if (mClient) {
        sendAllWatchVarsToDebugger();
        if (mDebuggerType==DebuggerType::LLDB_MI) {
            for (PWatchVar var:mWatchModel->watchVars()) {
                if (!var->name.isEmpty())
                    mClient->refreshWatch(var);
            }
        } else {
            mClient->refreshWatch();
        }
    }
}

void Debugger::fetchVarChildren(const QString &varName)
{
    if (mClient) {
        mClient->fetchWatchVarChildren(varName);
    }
}

bool Debugger::useDebugServer() const
{
    return mUseDebugServer;
}

void Debugger::setUseDebugServer(bool newUseDebugServer)
{
    mUseDebugServer = newUseDebugServer;
}

bool Debugger::supportDisassemlyBlendMode()
{
    return mDebuggerType == DebuggerType::GDB;
}

bool Debugger::debugInfosUsingUTF8() const
{
    return mDebugInfosUsingUTF8;
}

void Debugger::setDebugInfosUsingUTF8(bool newDebugInfosUsingUTF8)
{
    mDebugInfosUsingUTF8 = newDebugInfosUsingUTF8;
}

DebuggerType Debugger::debuggerType() const
{
    return mDebuggerType;
}

void Debugger::setDebuggerType(DebuggerType newDebuggerType)
{
    mDebuggerType = newDebuggerType;
}

bool Debugger::forceUTF8() const
{
    return mForceUTF8;
}

void Debugger::setForceUTF8(bool newForceUTF8)
{
    mForceUTF8 = newForceUTF8;
}

std::shared_ptr<MemoryModel> Debugger::memoryModel() const
{
    return mMemoryModel;
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

PWatchVar Debugger::watchVarAt(const QModelIndex &index)
{
    return mWatchModel->findWatchVar(index);
}

void Debugger::readMemory(const QString &startAddress, int rows, int cols)
{
    if (mClient)
        mClient->readMemory(startAddress, rows, cols);
}

void Debugger::evalExpression(const QString &expression)
{
    if (mClient)
        mClient->evalExpression(expression);
}

void Debugger::selectFrame(PTrace trace)
{
    if (mClient)
        mClient->selectFrame(trace);
}

void Debugger::refreshFrame()
{
    if (mClient) {
        mClient->refreshFrame();
    }
}

void Debugger::refreshStackVariables()
{
    if (mClient)
        mClient->refreshStackVariables();
}

void Debugger::refreshRegisters()
{
    if (mClient)
        mClient->refreshRegisters();
}

void Debugger::disassembleCurrentFrame(bool blendMode)
{
    if (mClient)
        mClient->disassembleCurrentFrame(blendMode);
}

void Debugger::setDisassemblyLanguage(bool isIntel)
{
    if (mClient)
        mClient->setDisassemblyLanguage(isIntel);
}

//void Debugger::notifyWatchVarUpdated(PWatchVar var)
//{
//    mWatchModel->notifyUpdated(var);
//}
std::shared_ptr<BacktraceModel> Debugger::backtraceModel()
{
    return mBacktraceModel;
}

std::shared_ptr<BreakpointModel> Debugger::breakpointModel()
{
    return mBreakpointModel;
}

void Debugger::sendWatchCommand(PWatchVar var)
{
    if (mClient)
        mClient->addWatch(var->expression);
}

void Debugger::sendRemoveWatchCommand(PWatchVar var)
{
    if (mClient)
        mClient->removeWatch(var);
}

void Debugger::sendBreakpointCommand(PBreakpoint breakpoint)
{
    if (mClient)
        mClient->addBreakpoint(breakpoint);
}

void Debugger::sendClearBreakpointCommand(int index, bool forProject)
{
    sendClearBreakpointCommand(mBreakpointModel->breakpoints(forProject)[index]);
}

void Debugger::sendClearBreakpointCommand(PBreakpoint breakpoint)
{
    if (mClient)
        mClient->removeBreakpoint(breakpoint);
}

QJsonArray BreakpointModel::toJson(const QString& projectFolder)
{
    bool forProject = !projectFolder.isEmpty();
    QJsonArray array;
    foreach (const PBreakpoint& breakpoint, breakpoints(forProject)) {
        QJsonObject obj;
        if (forProject)
            obj["filename"]=extractRelativePath(projectFolder, breakpoint->filename);
        else
            obj["filename"]=breakpoint->filename;
        obj["line"]=breakpoint->line;
        obj["condition"]=breakpoint->condition;
        obj["enabled"]=breakpoint->enabled;
        obj["breakpoint_type"] = static_cast<int>(breakpoint->breakpointType);
        obj["timestamp"]=QString("%1").arg(breakpoint->timestamp);
        array.append(obj);
    }
    return array;
}

void BreakpointModel::setBreakpoints(const QList<PBreakpoint> &list, bool forProject)
{
    if (mIsForProject == forProject)
        beginResetModel();
    if (forProject) {
        mProjectBreakpoints = list;
    } else {
        mBreakpoints = list;
    }
    if (mIsForProject == forProject)
        endResetModel();
}

void Debugger::save(const QString &filename, const QString& projectFolder)
{
    bool forProject=!projectFolder.isEmpty();
    QList<PBreakpoint> breakpoints;
    QList<PWatchVar> watchVars=mWatchModel->watchVars(forProject);
    QSet<QString> breakpointCompareSet;
    QSet<QString> watchVarCompareSet;
    if (forProject) {
        //convert project file's absolute path to relative path
        foreach (const PBreakpoint& breakpoint, mBreakpointModel->breakpoints(forProject)) {
            QString filename = extractRelativePath(projectFolder, breakpoint->filename);
            QString key = QString("%1-%2").arg(filename).arg(breakpoint->line);
            breakpointCompareSet.insert(key);
        }
    } else {
        foreach (const PBreakpoint& breakpoint, mBreakpointModel->breakpoints(forProject)) {
            QString key = QString("%1-%2").arg(breakpoint->filename).arg(breakpoint->line);
            breakpointCompareSet.insert(key);
        }
    }
    foreach (const PWatchVar& watchVar, watchVars) {
        watchVarCompareSet.insert(watchVar->expression);
    }
    std::shared_ptr<DebugConfig> pConfig;
    try {
        pConfig = load(filename, forProject);
    } catch (FileError& e) {

    }

    QFile file(filename);
    if (file.open(QFile::WriteOnly | QFile::Truncate)) {
        foreach (const PBreakpoint& breakpoint, pConfig->breakpoints) {
            QString key = QString("%1-%2").arg(breakpoint->filename).arg(breakpoint->line);
            if (!breakpointCompareSet.contains(key)) {
                breakpointCompareSet.insert(key);
                if (forProject)
                    breakpoint->filename=generateAbsolutePath(projectFolder,breakpoint->filename);
                mBreakpointModel->addBreakpoint(breakpoint,forProject);
            }
        }
        foreach (const PWatchVar& watchVar, pConfig->watchVars) {
            QString key = watchVar->expression;
            if (!watchVarCompareSet.contains(key)) {
                watchVarCompareSet.insert(key);
                addWatchVar(watchVar,forProject);
            }
        }
        qint64 saveTimestamp = QDateTime::currentMSecsSinceEpoch();;
        if (forProject) {
            mProjectLastLoadtime = saveTimestamp;
        } else {
            mLastLoadtime = saveTimestamp;
        }
        QJsonObject rootObj;
        rootObj["timestamp"] = QString("%1").arg(saveTimestamp);

        if (forProject) {
            rootObj["breakpoints"] = mBreakpointModel->toJson(projectFolder);
        }
        rootObj["watchvars"] = mWatchModel->toJson(forProject);
        QJsonDocument doc;
        doc.setObject(rootObj);
        if (file.write(doc.toJson())<0) {
            throw FileError(tr("Save file '%1' failed.")
                            .arg(filename));
        }
    } else {
        throw FileError(tr("Can't open file '%1' for write.")
                        .arg(filename));
    }
}

PDebugConfig Debugger::load(const QString &filename, bool forProject)
{
    qint64 criteriaTimestamp;
    if (forProject) {
        criteriaTimestamp = mProjectLastLoadtime;
    } else {
        criteriaTimestamp = mLastLoadtime;
    }
    std::shared_ptr<DebugConfig> pConfig=std::make_shared<DebugConfig>();
    pConfig->timestamp=0;
    QFile file(filename);
    if (!file.exists())
        return pConfig;
    if (file.open(QFile::ReadOnly)) {
        QByteArray content = file.readAll().trimmed();
        if (content.isEmpty())
            return pConfig;
        QJsonParseError error;        
        QJsonDocument doc(QJsonDocument::fromJson(content,&error));
        if (error.error  != QJsonParseError::NoError) {
            throw FileError(tr("Error in json file '%1':%2 : %3")
                            .arg(filename)
                            .arg(error.offset)
                            .arg(error.errorString()));
        }
        QJsonObject rootObject = doc.object();
        qint64 timestamp = rootObject["timestamp"].toString().toLongLong();
        if (timestamp <= criteriaTimestamp)
            return pConfig;
        pConfig->timestamp = timestamp;

        pConfig->breakpoints = mBreakpointModel->loadJson(rootObject["breakpoints"].toArray(),criteriaTimestamp);
        pConfig->watchVars = mWatchModel->loadJson(rootObject["watchvars"].toArray(), criteriaTimestamp);
        if (forProject) {
            mProjectLastLoadtime = QDateTime::currentMSecsSinceEpoch();
        } else {
            mLastLoadtime = QDateTime::currentMSecsSinceEpoch();
        }
    } else {
        throw FileError(tr("Can't open file '%1' for read.")
                        .arg(filename));
    }
    return pConfig;
}

void Debugger::addWatchVar(const PWatchVar &watchVar, bool forProject)
{
    mWatchModel->addWatchVar(watchVar,forProject);
    if (forProject == isForProject())
        sendWatchCommand(watchVar);
}

void Debugger::includeOrSkipDirsInSymbolSearch(const QStringList &dirs, bool skip)
{
    if (skip) {
        mClient->skipDirectoriesInSymbolSearch(dirs);
    } else {
        mClient->addSymbolSearchDirectories(dirs);
    }
}

void Debugger::syncFinishedParsing()
{
    bool spawnedcpuform = false;

    // GDB determined that the source code is more recent than the executable. Ask the user if he wants to rebuild.
    if (mClient->receivedSFWarning()) {
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
            for (const QString& line:mClient->fullOutput()) {
                pMainWindow->addDebugOutput(line);
            }
            //pMainWindow->addDebugOutput("(gdb)");
        } else {
            // if (mClient->currentCmd() && mClient->currentCmd()->command == "disas") {

            // } else {
            //     for (const QString& line:mClient->consoleOutput()) {
            //         pMainWindow->addDebugOutput(line);
            //     }
            //     if (
            //             (mClient->currentCmd()
            //              && mClient->currentCmd()->source== DebugCommandSource::Console)
            //             || !mClient->consoleOutput().isEmpty() ) {
            //         pMainWindow->addDebugOutput("(gdb)");
            //     }
            // }
            if (!mClient->consoleOutput().isEmpty()) {
                for (const QString& line:mClient->consoleOutput()) {
                    pMainWindow->addDebugOutput(line);
                }
                pMainWindow->addDebugOutput("(gdb)");
            }
        }
    }

    // The program to debug has stopped. Stop the debugger
    if (mClient->processExited()) {
        stop();
        return;
    }

    if (mClient->signalReceived()
            && mClient->signalName()!="SIGINT"
            && mClient->signalName()!="SIGTRAP") {
        SignalMessageDialog dialog(pMainWindow);
        dialog.setOpenCPUInfo(pSettings->debugger().openCPUInfoWhenSignaled());
        dialog.setMessage(
                    tr("Signal \"%1\" Received: ").arg(mClient->signalName())
                    + "<br />"
                    + mClient->signalMeaning());
        int result = dialog.exec();
        if (result == QDialog::Accepted && dialog.openCPUInfo()) {
            pMainWindow->showCPUInfoDialog();
        }
    }

    // CPU form updates itself when spawned, don't update twice!
    if ((mClient->updateCPUInfo() && !spawnedcpuform) && (pMainWindow->cpuDialog()!=nullptr)) {
        pMainWindow->cpuDialog()->updateInfo();
    }
}

void Debugger::setMemoryData(qulonglong address, unsigned char data)
{
    if (mClient)
        mClient->writeMemory(address, data);
    refreshAll();
}

void Debugger::setWatchVarValue(const QString &name, const QString &value)
{
    if (mClient)
        mClient->writeWatchVar(name, value);
    refreshAll();
}

void Debugger::updateMemory(const QStringList &value)
{
    mMemoryModel->updateMemory(value);
    emit memoryExamineReady(value);
}

void Debugger::updateEval(const QString &value)
{
    emit evalValueReady(value);
}

void Debugger::updateDisassembly(const QString& file, const QString& func, const QStringList &value)
{
    if (pMainWindow->cpuDialog()) {
        pMainWindow->cpuDialog()->setDisassembly(file,func,value,mBacktraceModel->backtraces());
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

DebuggerClient::DebuggerClient(Debugger* debugger, QObject *parent) : QThread(parent),
    mCmdQueueMutex(),
    mStartSemaphore(0)
{
    mDebugger = debugger;
    mCmdRunning = false;
}

const QStringList &DebuggerClient::binDirs() const
{
    return mBinDirs;
}

void DebuggerClient::addBinDirs(const QStringList &binDirs)
{
    mBinDirs.append(binDirs);
}

void DebuggerClient::addBinDir(const QString &binDir)
{
    mBinDirs.append(binDir);
}

const QString &DebuggerClient::signalMeaning() const
{
    return mSignalMeaning;
}

const QString &DebuggerClient::signalName() const
{
    return mSignalName;
}

bool DebuggerClient::inferiorRunning() const
{
    return mInferiorRunning;
}

const QStringList &DebuggerClient::fullOutput() const
{
    return mFullOutput;
}

bool DebuggerClient::receivedSFWarning() const
{
    return mReceivedSFWarning;
}

bool DebuggerClient::updateCPUInfo() const
{
    return mUpdateCPUInfo;
}

const QStringList &DebuggerClient::consoleOutput() const
{
    return mConsoleOutput;
}

bool DebuggerClient::signalReceived() const
{
    return mSignalReceived;
}

bool DebuggerClient::processExited() const
{
    return mProcessExited;
}

QString DebuggerClient::debuggerPath() const
{
    return mDebuggerPath;
}

void DebuggerClient::setDebuggerPath(const QString &debuggerPath)
{
    mDebuggerPath = debuggerPath;
}

void DebuggerClient::waitStart()
{
    mStartSemaphore.acquire(1);
}


BreakpointModel::BreakpointModel(QObject *parent):QAbstractTableModel(parent),
    mIsForProject(false)
{

}

int BreakpointModel::rowCount(const QModelIndex &) const
{
    return breakpoints(mIsForProject).size();
}

int BreakpointModel::columnCount(const QModelIndex &) const
{
    return 3;
}

QVariant BreakpointModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    const QList<PBreakpoint> &list=breakpoints(mIsForProject);
    if (index.row()<0 || index.row() >= static_cast<int>(list.size()))
        return QVariant();

    PBreakpoint breakpoint = list[index.row()];
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

void BreakpointModel::addBreakpoint(PBreakpoint p, bool forProject)
{

    if (forProject) {
        if (forProject==mIsForProject)
            beginInsertRows(QModelIndex(),mProjectBreakpoints.count(),mProjectBreakpoints.count());
        mProjectBreakpoints.push_back(p);
    } else {
        if (forProject==mIsForProject)
            beginInsertRows(QModelIndex(),mBreakpoints.count(),mBreakpoints.count());
        mBreakpoints.push_back(p);
    }
    if (forProject==mIsForProject)
        endInsertRows();
}

void BreakpointModel::clear(bool forProject)
{
    if (forProject == mIsForProject)
        beginResetModel();
    if (forProject)
        mProjectBreakpoints.clear();
    else
        mBreakpoints.clear();
    if (forProject == mIsForProject)
        endResetModel();
}

void BreakpointModel::removeBreakpoint(int row, bool forProject)
{
    if (forProject==mIsForProject)
        beginRemoveRows(QModelIndex(),row,row);
    if (forProject)
        mProjectBreakpoints.removeAt(row);
    else
        mBreakpoints.removeAt(row);
    if (forProject==mIsForProject)
        endRemoveRows();
}

void BreakpointModel::removeBreakpointsInFile(const QString &fileName, bool forProject)
{
    QList<PBreakpoint> & lst=forProject?mProjectBreakpoints:mBreakpoints;
    for (int i=lst.count()-1;i>=0;i--) {
        if (lst[i]->filename==fileName)
            removeBreakpoint(i,forProject);
    }
}

void BreakpointModel::renameBreakpointFilenames(const QString &oldFileName, const QString &newFileName, bool forProject)
{
    QList<PBreakpoint> & lst=forProject?mProjectBreakpoints:mBreakpoints;
    for (int i=lst.count()-1;i>=0;i--) {
        if (lst[i]->filename==oldFileName) {
            lst[i]->filename=newFileName;
            if (forProject == mIsForProject) {
                QModelIndex index=createIndex(i,0);
                emit dataChanged(index,index);
            }
        }
    }
}

void BreakpointModel::invalidateAllBreakpointNumbers()
{
    foreach (PBreakpoint bp,mBreakpoints) {
        bp->number = -1;
    }
    foreach (PBreakpoint bp,mProjectBreakpoints) {
        bp->number = -1;
    }
    //emit dateChanged(createIndex(0,0),)
}

PBreakpoint BreakpointModel::setBreakPointCondition(int index, const QString &condition,bool forProject)
{
    PBreakpoint breakpoint = breakpoints(forProject)[index];
    breakpoint->condition = condition;
    if (forProject==mIsForProject)
        emit dataChanged(createIndex(index,0),createIndex(index,2));
    return breakpoint;
}

PBreakpoint BreakpointModel::breakpoint(int index, bool forProject) const
{
    const QList<PBreakpoint> list=breakpoints(forProject);
    if (index<0 && index>=list.count())
        return PBreakpoint();
    return list[index];
}


void BreakpointModel::updateBreakpointNumber(const QString& filename, int line, int number)
{
    foreach (PBreakpoint bp, breakpoints(mIsForProject)) {
        if (bp->filename == filename && bp->line == line) {
            bp->number = number;
            return;
        }
    }
}

void BreakpointModel::onFileDeleteLines(const QString& filename, int startLine, int count, bool forProject)
{
    const QList<PBreakpoint> &list=breakpoints(forProject);
    for (int i = list.count()-1;i>=0;i--){
        PBreakpoint breakpoint = list[i];
        if  (breakpoint->filename == filename
             && breakpoint->line>=startLine) {
            if (breakpoint->line >= startLine+count) {
                breakpoint->line -= count;
                if (forProject==mIsForProject)
                    emit dataChanged(createIndex(i,0),createIndex(i,2));
            } else {
                removeBreakpoint(i,forProject);
            }
        }
    }
}

void BreakpointModel::onFileInsertLines(const QString& filename, int startLine, int count, bool forProject)
{
    const QList<PBreakpoint> &list=breakpoints(forProject);
    for (int i = list.count()-1;i>=0;i--){
        PBreakpoint breakpoint = list[i];
        if  (breakpoint->filename == filename
             && breakpoint->line>=startLine) {
            breakpoint->line+=count;
            if (forProject == mIsForProject)
                emit dataChanged(createIndex(i,0),createIndex(i,2));
        }
    }
}

bool BreakpointModel::isForProject() const
{
    return mIsForProject;
}

void BreakpointModel::setIsForProject(bool newIsForProject)
{
    if (mIsForProject!=newIsForProject) {
        beginResetModel();
        mIsForProject = newIsForProject;
        endResetModel();
    }
}

QList<PBreakpoint> BreakpointModel::loadJson(const QJsonArray& jsonArray, qint64 criteriaTime)
{
    QList<PBreakpoint> result;

    for  (int i=0;i<jsonArray.count();i++) {
        QJsonValue value = jsonArray[i];
        QJsonObject obj=value.toObject();
        bool ok;
        qint64 timestamp = obj["timestamp"].toString().toLongLong(&ok);

        if (ok && timestamp > criteriaTime) {
            PBreakpoint breakpoint = std::make_shared<Breakpoint>();
            breakpoint->filename = obj["filename"].toString();
            breakpoint->line = obj["line"].toInt();
            breakpoint->condition = obj["condition"].toString();
            breakpoint->enabled = obj["enabled"].toBool();
            breakpoint->breakpointType = static_cast<BreakpointType>(obj["breakpoint_type"].toInt());
            breakpoint->timestamp = timestamp;
            result.append(breakpoint);
        }
    }

    return result;
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
    case Qt::ToolTipRole:
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
    mIsForProject = false;
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
        pChild = watchVars(mIsForProject)[row];
    } else {
        parentItem = static_cast<WatchVar*>(parent.internalPointer());
        pChild = parentItem->children[row];
    }
    if (pChild) {
        return createIndex(row,column,pChild.get());
    }
    return QModelIndex();
}

static int getWatchIndex(WatchVar* var, const QList<PWatchVar> &list) {
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
    PWatchVar parentItem = childItem->parent.lock();
    //parent is root
    if (parentItem == nullptr) {
        return QModelIndex();
    }
    int row;
    PWatchVar grandItem = parentItem->parent.lock();
    if (grandItem == nullptr) {
        row = getWatchIndex(parentItem.get(), watchVars(mIsForProject));
    } else {
        row = getWatchIndex(parentItem.get(), grandItem->children);
    }
    return createIndex(row,0,parentItem.get());
}

int WatchModel::rowCount(const QModelIndex &parent) const
{
    if (!parent.isValid()) {
        return watchVars(mIsForProject).count();
    } else {
        WatchVar* parentItem = static_cast<WatchVar*>(parent.internalPointer());
        return parentItem->children.count();
    }
}

int WatchModel::columnCount(const QModelIndex&) const
{
    return 3;
}

void WatchModel::addWatchVar(PWatchVar watchVar, bool forProject)
{
    QList<PWatchVar> &vars=(forProject?mProjectWatchVars:mWatchVars);
    for (PWatchVar var:vars) {
        if (watchVar->expression == var->expression) {
            return;
        }
    }
    if (forProject==mIsForProject)
        beginInsertRows(QModelIndex(),vars.count(),vars.count());
    vars.append(watchVar);
    if (forProject==mIsForProject)
        endInsertRows();
}

void WatchModel::setWatchVars(const QList<PWatchVar> list, bool forProject)
{
    if (mIsForProject == forProject)
        beginResetModel();
    if (forProject) {
        mProjectWatchVars = list;
    } else {
        mWatchVars = list;
    }
    if (mIsForProject == forProject)
        endResetModel();
}

void WatchModel::removeWatchVar(const QString &express)
{
    QList<PWatchVar> &vars=(mIsForProject?mProjectWatchVars:mWatchVars);
    for (int i=vars.size()-1;i>=0;i--) {
        PWatchVar var = vars[i];
        if (express == var->expression) {
            QModelIndex  parentIndex = index(var->parent.lock());
            beginRemoveRows(parentIndex,i,i);
            if (mVarIndex.contains(var->name))
                mVarIndex.remove(var->name);
            vars.removeAt(i);
            endRemoveRows();
        }
    }
}

void WatchModel::removeWatchVar(const QModelIndex &index)
{
    int r=index.row();
    beginRemoveRows(QModelIndex(),r,r);
    QList<PWatchVar> &vars=(mIsForProject?mProjectWatchVars:mWatchVars);
    PWatchVar var = vars[r];
    if (mVarIndex.contains(var->name))
        mVarIndex.remove(var->name);
    vars.removeAt(r);
    endRemoveRows();
}

void WatchModel::clear()
{
    beginResetModel();
    QList<PWatchVar> &vars=(mIsForProject?mProjectWatchVars:mWatchVars);
    vars.clear();
    endResetModel();
}

void WatchModel::clear(bool forProject)
{
    if (mIsForProject == forProject)
        beginResetModel();
    QList<PWatchVar> &vars=(forProject?mProjectWatchVars:mWatchVars);
    vars.clear();
    if (mIsForProject == forProject)
        endResetModel();
}

const QList<PWatchVar> &WatchModel::watchVars() const
{
    return watchVars(mIsForProject);
}

PWatchVar WatchModel::findWatchVar(const QModelIndex &index)
{
    if (!index.isValid())
        return PWatchVar();
    int r=index.row();
    return watchVars(mIsForProject)[r];
}

PWatchVar WatchModel::findWatchVar(const QString &expr)
{
    foreach (const PWatchVar &var, watchVars(mIsForProject)) {
        if (expr == QString("\"%1\"").arg(var->expression)) {
            return var;
        }
    }
    return PWatchVar();
}

void WatchModel::resetAllVarInfos()
{
    beginResetModel();
    foreach (PWatchVar var, watchVars(mIsForProject)) {
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
    child->parent = var;
    child->timestamp = QDateTime::currentMSecsSinceEpoch();
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

void WatchModel::updateAllHasMoreVars()
{
    foreach (const PWatchVar& var, mVarIndex.values()) {
        if (var->hasMore) {
            QModelIndex idx = index(var);
            fetchMore(idx);
        }
    }
}

bool WatchModel::isForProject() const
{
    return mIsForProject;
}

void WatchModel::setIsForProject(bool newIsForProject)
{
    if (newIsForProject!=mIsForProject) {
        beginResetModel();
        mVarIndex.clear();
        mIsForProject=newIsForProject;
        endResetModel();
    }
}

const QList<PWatchVar> &WatchModel::watchVars(bool forProject) const
{
    return forProject?mProjectWatchVars:mWatchVars;
}

void WatchModel::clearAllVarInfos()
{
    beginResetModel();
    foreach (PWatchVar var, watchVars(mIsForProject)) {
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
    PWatchVar parent = var->parent.lock();
    if (parent==nullptr) {
        row = watchVars(mIsForProject).indexOf(var);
    } else {
        row = parent->children.indexOf(var);
    }
    if (row<0)
        return;
    //qDebug()<<"dataChanged"<<row<<":"<<var->text;
    emit dataChanged(createIndex(row,0,var.get()),createIndex(row,0,var.get()));
}

QJsonArray WatchModel::toJson(bool forProject)
{
    QJsonArray array;
    foreach (const PWatchVar& watchVar, watchVars(forProject)) {
        QJsonObject obj;
        obj["expression"]=watchVar->expression;
        obj["timestamp"]=QString("%1").arg(watchVar->timestamp);
        array.append(obj);
    }
    return array;
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
    PWatchVar parent=pVar->parent.lock();
    if (parent) {
        int row=-1;
        for (int i=0;i<parent->children.count();i++) {
            if (parent->children[i].get() == pVar) {
                row = i;
                break;
            }
        }
        if (row<0)
            return QModelIndex();
        return createIndex(row,0,pVar);
    } else {
        const QList<PWatchVar> &vars=watchVars(mIsForProject);
        int row=-1;
        for (int i=0;i<vars.count();i++) {
            if (vars[i].get() == pVar) {
                row = i;
                break;
            }
        }
        if (row<0)
            return QModelIndex();
        return createIndex(row,0,pVar);
    }
}

QList<PWatchVar> WatchModel::loadJson(const QJsonArray &jsonArray, qint64 criteriaTimestamp)
{

    QList<PWatchVar> result;
    QJsonArray array = jsonArray;
    for  (int i=0;i<jsonArray.count();i++) {
        QJsonValue value = array[i];
        QJsonObject obj=value.toObject();
        bool ok;
        qint64 timestamp = obj["timestamp"].toString().toLongLong(&ok);
        if (ok && timestamp>criteriaTimestamp) {
            PWatchVar var = std::make_shared<WatchVar>();
            var->parent= PWatchVar();
            var->expression = obj["expression"].toString();
            var->value = tr("Execute to evaluate");
            var->numChild = 0;
            var->hasMore=false;
            var->timestamp = timestamp;
            result.append(var);
        }
    }
    return result;
}

bool WatchModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid()) {
        return false;
    }
    if (index.column()==2 && role == Qt::EditRole) {
        WatchVar* item = static_cast<WatchVar*>(index.internalPointer());
        emit setWatchVarValue(item->name,value.toString());
    }
    return false;

}

Qt::ItemFlags WatchModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (!index.isValid()) {
        return Qt::ItemIsEnabled;
    }
    if (index.column() == 2) {
        WatchVar* item = static_cast<WatchVar*>(index.internalPointer());
        if (item->numChild==0 && !item->type.isEmpty())
            flags |= Qt::ItemIsEditable;
    }
    return flags;
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
    return item->numChild>0 || item->hasMore;
}

RegisterModel::RegisterModel(QObject *parent):QAbstractTableModel(parent)
{
#if defined(ARCH_X86_64) || defined(ARCH_X86)
    //https://www.intel.com/content/www/us/en/developer/articles/technical/intel-sdm.html
        mRegisterDescriptions.insert("rax",tr("64-bit")+" "+tr("Accumulator for operands and results data"));
        mRegisterDescriptions.insert("rbx",tr("64-bit")+" "+tr("Pointer to data in the DS segment"));
        mRegisterDescriptions.insert("rcx",tr("64-bit")+" "+tr("Counter for string and loop operations"));
        mRegisterDescriptions.insert("rdx",tr("64-bit")+" "+tr("I/O pointer"));
        mRegisterDescriptions.insert("rsi",tr("64-bit")+" "+tr("Source index for string operations; Pointer to data in the segment pointed to by the DS register"));
        mRegisterDescriptions.insert("rdi",tr("64-bit")+" "+tr("Destination index for string operations; Pointer to data (or destination) in the segment pointed to by the ES register"));
        mRegisterDescriptions.insert("rsp",tr("64-bit")+" "+tr("Stack pointer (in the SS segment)"));
        mRegisterDescriptions.insert("rbp",tr("64-bit")+" "+tr("Pointer to data on the stack (in the SS segment)"));
        mRegisterDescriptions.insert("r8",tr("64-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r9",tr("64-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r10",tr("64-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r11",tr("64-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r12",tr("64-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r13",tr("64-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r14",tr("64-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r15",tr("64-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("rip",tr("64-bit")+" "+tr("Instruction pointer"));
        mRegisterDescriptions.insert("rflags",tr("Flags"));
        mRegisterDescriptions.insert("eflags",tr("Flags"));

        mRegisterDescriptions.insert("eax",tr("32-bit")+" "+tr("Accumulator for operands and results data"));
        mRegisterDescriptions.insert("ebx",tr("32-bit")+" "+tr("Pointer to data in the DS segment"));
        mRegisterDescriptions.insert("ecx",tr("32-bit")+" "+tr("Counter for string and loop operations"));
        mRegisterDescriptions.insert("edx",tr("32-bit")+" "+tr("I/O pointer"));
        mRegisterDescriptions.insert("esi",tr("32-bit")+" "+tr("Source index for string operations; Pointer to data in the segment pointed to by the DS register"));
        mRegisterDescriptions.insert("edi",tr("32-bit")+" "+tr("Destination index for string operations; Pointer to data (or destination) in the segment pointed to by the ES register"));
        mRegisterDescriptions.insert("esp",tr("32-bit")+" "+tr("Stack pointer (in the SS segment)"));
        mRegisterDescriptions.insert("ebp",tr("32-bit")+" "+tr("Pointer to data on the stack (in the SS segment)"));
        mRegisterDescriptions.insert("r8d",tr("32-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r9d",tr("32-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r10d",tr("32-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r11d",tr("32-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r12d",tr("32-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r13d",tr("32-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r14d",tr("32-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("r15d",tr("32-bit")+" "+tr("General purpose"));
        mRegisterDescriptions.insert("eip",tr("32-bit")+" "+tr("Instruction pointer"));

        mRegisterDescriptions.insert("ax",tr("lower 16 bits of %1").arg("rax/eax"));
        mRegisterDescriptions.insert("bx",tr("lower 16 bits of %1").arg("rbx/rbx"));
        mRegisterDescriptions.insert("cx",tr("lower 16 bits of %1").arg("rcx/ecx"));
        mRegisterDescriptions.insert("dx",tr("lower 16 bits of %1").arg("rdx/edx"));
        mRegisterDescriptions.insert("si",tr("lower 16 bits of %1").arg("rsi/esi"));
        mRegisterDescriptions.insert("di",tr("lower 16 bits of %1").arg("rdi/edi"));
        mRegisterDescriptions.insert("sp",tr("lower 16 bits of %1").arg("rsp/esp"));
        mRegisterDescriptions.insert("bp",tr("lower 16 bits of %1").arg("rbp/esp"));
        mRegisterDescriptions.insert("r8w",tr("lower 16 bits of %1").arg("r8"));
        mRegisterDescriptions.insert("r9w",tr("lower 16 bits of %1").arg("r9"));
        mRegisterDescriptions.insert("r10w",tr("lower 16 bits of %1").arg("r10"));
        mRegisterDescriptions.insert("r11w",tr("lower 16 bits of %1").arg("r11"));
        mRegisterDescriptions.insert("r12w",tr("lower 16 bits of %1").arg("r12"));
        mRegisterDescriptions.insert("r13w",tr("lower 16 bits of %1").arg("r13"));
        mRegisterDescriptions.insert("r14w",tr("lower 16 bits of %1").arg("r14"));
        mRegisterDescriptions.insert("r15w",tr("lower 16 bits of %1").arg("r15"));
        mRegisterDescriptions.insert("ip",tr("lower 16 bits of %1").arg("rip/eip"));

        mRegisterDescriptions.insert("al",tr("lower 8 bits of %1").arg("rax/eax"));
        mRegisterDescriptions.insert("bl",tr("lower 8 bits of %1").arg("rbx/rbx"));
        mRegisterDescriptions.insert("cl",tr("lower 8 bits of %1").arg("rcx/ecx"));
        mRegisterDescriptions.insert("dl",tr("lower 8 bits of %1").arg("rdx/edx"));
        mRegisterDescriptions.insert("sil",tr("lower 8 bits of %1").arg("rsi/esi"));
        mRegisterDescriptions.insert("dil",tr("lower 8 bits of %1").arg("rdi/edi"));
        mRegisterDescriptions.insert("spl",tr("lower 8 bits of %1").arg("rsp/esp"));
        mRegisterDescriptions.insert("bpl",tr("lower 8 bits of %1").arg("rbp/esp"));
        mRegisterDescriptions.insert("r8b",tr("lower 8 bits of %1").arg("r8"));
        mRegisterDescriptions.insert("r9b",tr("lower 8 bits of %1").arg("r9"));
        mRegisterDescriptions.insert("r10b",tr("lower 8 bits of %1").arg("r10"));
        mRegisterDescriptions.insert("r11b",tr("lower 8 bits of %1").arg("r11"));
        mRegisterDescriptions.insert("r12b",tr("lower 8 bits of %1").arg("r12"));
        mRegisterDescriptions.insert("r13b",tr("lower 8 bits of %1").arg("r13"));
        mRegisterDescriptions.insert("r14b",tr("lower 8 bits of %1").arg("r14"));
        mRegisterDescriptions.insert("r15b",tr("lower 8 bits of %1").arg("r15"));

        mRegisterDescriptions.insert("ah",tr("8 high bits of lower 16 bits of %1").arg("rax/eax"));
        mRegisterDescriptions.insert("bh",tr("8 high bits of lower 16 bits of %1").arg("rbx/rbx"));
        mRegisterDescriptions.insert("ch",tr("8 high bits of lower 16 bits of %1").arg("rcx/ecx"));
        mRegisterDescriptions.insert("dh",tr("8 high bits of lower 16 bits of %1").arg("rdx/edx"));

        mRegisterDescriptions.insert("cs",tr("16-bit")+" "+tr("Code segment selector"));
        mRegisterDescriptions.insert("ds",tr("16-bit")+" "+tr("Data segment selector"));
        mRegisterDescriptions.insert("es",tr("16-bit")+" "+tr("Extra data segment selector"));
        mRegisterDescriptions.insert("fs",tr("16-bit")+" "+tr("Extra data segment selector"));
        mRegisterDescriptions.insert("gs",tr("16-bit")+" "+tr("Extra data segment selector"));
        mRegisterDescriptions.insert("ss",tr("16-bit")+" "+tr("Stack segment selector"));

//x87 fpu
        mRegisterDescriptions.insert("st0",tr("Floating-point data"));
        mRegisterDescriptions.insert("st1",tr("Floating-point data"));
        mRegisterDescriptions.insert("st2",tr("Floating-point data"));
        mRegisterDescriptions.insert("st3",tr("Floating-point data"));
        mRegisterDescriptions.insert("st4",tr("Floating-point data"));
        mRegisterDescriptions.insert("st5",tr("Floating-point data"));
        mRegisterDescriptions.insert("st6",tr("Floating-point data"));
        mRegisterDescriptions.insert("st7",tr("Floating-point data"));

        mRegisterDescriptions.insert("fctrl",tr("Floating-point control"));
        mRegisterDescriptions.insert("fstat",tr("Floating-point status"));
        mRegisterDescriptions.insert("ftag",tr("Floating-point tag word"));
        mRegisterDescriptions.insert("fop",tr("Floating-point operation"));
        mRegisterDescriptions.insert("fiseg",tr("Floating-point last instruction segment"));
        mRegisterDescriptions.insert("fioff",tr("Floating-point last instruction offset"));
        mRegisterDescriptions.insert("foseg",tr("Floating-point last operand segment"));
        mRegisterDescriptions.insert("fooff",tr("Floating-point last operand offset"));

        mRegisterDescriptions.insert("mm0",tr("64-bit")+" "+"MMX");
        mRegisterDescriptions.insert("mm1",tr("64-bit")+" "+"MMX");
        mRegisterDescriptions.insert("mm2",tr("64-bit")+" "+"MMX");
        mRegisterDescriptions.insert("mm3",tr("64-bit")+" "+"MMX");
        mRegisterDescriptions.insert("mm4",tr("64-bit")+" "+"MMX");
        mRegisterDescriptions.insert("mm5",tr("64-bit")+" "+"MMX");
        mRegisterDescriptions.insert("mm6",tr("64-bit")+" "+"MMX");
        mRegisterDescriptions.insert("mm7",tr("64-bit")+" "+"MMX");

        mRegisterDescriptions.insert("xmm0",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm1",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm2",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm3",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm4",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm5",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm6",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm7",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm8",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm9",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm11",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm12",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm13",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm14",tr("128-bit")+" "+"XMM");
        mRegisterDescriptions.insert("xmm15",tr("128-bit")+" "+"XMM");

        mRegisterDescriptions.insert("ymm0",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm1",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm2",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm3",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm4",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm5",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm6",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm7",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm8",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm9",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm11",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm12",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm13",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm14",tr("256-bit")+" "+"YMM");
        mRegisterDescriptions.insert("ymm15",tr("256-bit")+" "+"YMM");

        mRegisterDescriptions.insert("mxscr",tr("SSE status and control"));

#endif
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
            return mRegisterValues.value(
                        mRegisterNameIndex.value(index.row(),-1)
                        ,"");
        }
        break;
    case Qt::FontRole:
        return QFont{pSettings->debugger().fontName(),pSettings->debugger().fontSize()};
    case Qt::ToolTipRole:
        switch (index.column()) {
        case 0:
            return mRegisterDescriptions.value(mRegisterNames[index.row()],"");
        case 1:
            return mRegisterValues.value(
                        mRegisterNameIndex.value(index.row(),-1)
                        ,"");
        }
        break;
    default:
        break;
    }
    return QVariant();
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
    mRegisterNameIndex.clear();
    mRegisterNames.clear();
    for (int i=0;i<regNames.length();i++) {
        QString regName = regNames[i].trimmed();
        if (!regName.isEmpty()) {
            mRegisterNames.append(regNames[i]);
            mRegisterNameIndex.insert(mRegisterNames.count()-1,i);
        }
    }
    endResetModel();
}

void RegisterModel::updateValues(const QHash<int, QString> registerValues)
{
    for(auto it = registerValues.begin();it!=registerValues.end();++it) {
        int row = it.key();
        mRegisterValues[row] = it.value();
    }
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
        const QStringList& arguments,
        QObject *parent):
    QThread(parent),
    mInferior(inferior),
    mArguments(arguments),
    mGDBServer(GDBServer),
    mPort(port),
    mStop(false),
    mStartSemaphore(0),
    mErrorOccured(false)
{
    mProcess = nullptr;
}

void DebugTarget::setInputFile(const QString &inputFile)
{
    mInputFile = inputFile;
}

void DebugTarget::stopDebug()
{
    mStop = true;
}

void DebugTarget::waitStart()
{
    mStartSemaphore.acquire(1);
}

const QStringList &DebugTarget::binDirs() const
{
    return mBinDirs;
}

void DebugTarget::addBinDirs(const QStringList &binDirs)
{
    mBinDirs.append(binDirs);
}

void DebugTarget::addBinDir(const QString &binDir)
{
    mBinDirs.append(binDir);
}

void DebugTarget::run()
{
    mStop = false;
    mErrorOccured = false;

    //find first available port
    QStringList execArgs;
    if (mGDBServer.endsWith(LLDB_SERVER_PROGRAM))
        execArgs = QStringList{
            mGDBServer,
            "gdbserver",
            QString("localhost:%1").arg(mPort),
            //mInferior,
        } + mArguments;
    else
        execArgs = QStringList{
            mGDBServer,
            QString("localhost:%1").arg(mPort),
            mInferior,
        } + mArguments;
    QString cmd;
    QStringList arguments;
    PNonExclusiveTemporaryFileOwner fileOwner;
#ifdef Q_OS_WIN
    if (pSettings->environment().useCustomTerminal()) {
        std::tie(cmd, arguments, fileOwner) = wrapCommandForTerminalEmulator(
            pSettings->environment().terminalPath(),
            pSettings->environment().terminalArgumentsPattern(),
            execArgs
        );
    } else {
        cmd = execArgs[0];
        arguments = execArgs.mid(1);
    }
#else
    std::tie(cmd, arguments, fileOwner) = wrapCommandForTerminalEmulator(
        pSettings->environment().terminalPath(),
        pSettings->environment().terminalArgumentsPattern(),
        execArgs
    );
#endif
    QString workingDir = QFileInfo(mInferior).path();

    mProcess = std::make_shared<QProcess>();
    auto action = finally([&]{
        mProcess.reset();
    });
    mProcess->setProgram(cmd);
    mProcess->setArguments(arguments);
    mProcess->setProcessChannelMode(QProcess::MergedChannels);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    QStringList pathAdded = mBinDirs;
    if (!path.isEmpty()) {
        path = pathAdded.join(PATH_SEPARATOR) + PATH_SEPARATOR + path;
    } else {
        path = pathAdded.join(PATH_SEPARATOR);
    }
    QString cmdDir = extractFileDir(cmd);
    if (!cmdDir.isEmpty()) {
        path = cmdDir + PATH_SEPARATOR + path;
    }
    env.insert("PATH",path);
    mProcess->setProcessEnvironment(env);
    mProcess->setWorkingDirectory(workingDir);

#ifdef Q_OS_WIN
    mProcess->setCreateProcessArgumentsModifier([this](QProcess::CreateProcessArguments * args){
        if (programHasConsole(mInferior)) {
            args->flags |=  CREATE_NEW_CONSOLE;
            args->flags &= ~CREATE_NO_WINDOW;
        }
        if (mInputFile.isEmpty()) {
            args->startupInfo -> dwFlags &= ~STARTF_USESTDHANDLES;
        } else {
            args->startupInfo->hStdOutput = NULL;
            args->startupInfo->hStdError = NULL;
        }

    });
#endif

    connect(mProcess.get(), &QProcess::errorOccurred,
                    [&](){
                        mErrorOccured= true;
                    });
    mProcess->start();
    mProcess->waitForStarted(5000);
    mStartSemaphore.release(1);
    if (mProcess->state()==QProcess::Running && !mInputFile.isEmpty()) {
        mProcess->write(readFileToByteArray(mInputFile));
        mProcess->waitForFinished(0);
    }
    bool writeChannelClosed = false;
    while (true) {
        if (mProcess->bytesToWrite()==0 && !writeChannelClosed) {
            writeChannelClosed = true;
            mProcess->closeWriteChannel();
        }
        mProcess->waitForFinished(1);
        if (mProcess->state()!=QProcess::Running) {
            break;
        }
        if (mStop) {
            mProcess->terminate();
            mProcess->kill();
            break;
        }
        if (mErrorOccured)
            break;
        msleep(1);
    }
    if (mErrorOccured) {
        emit processFailed(mProcess->error());
    }
}

MemoryModel::MemoryModel(int dataPerLine, QObject *parent):
    QAbstractTableModel(parent),
    mDataPerLine(dataPerLine),
    mStartAddress(0)
{
}

void MemoryModel::updateMemory(const QStringList &value)
{
    int maxDataPerLine=-1;
    QRegularExpression delimiter("(\\s+)");
    QList<PMemoryLine> newModel;
    for (int i=0;i<value.length();i++) {
        QString line = value[i].trimmed();
        QStringList dataLst = line.split(delimiter,Qt::SkipEmptyParts);
        PMemoryLine memoryLine = std::make_shared<MemoryLine>();
        memoryLine->startAddress = -1;
        if (dataLst.length()>0) {
            bool isOk;
            memoryLine->startAddress = stringToHex(dataLst[0],isOk);
            if (isOk)  {
                if (dataLst.length()-1>maxDataPerLine)
                     maxDataPerLine = dataLst.length()-1;
                for (int j=1;j<dataLst.length();j++) {
                    qulonglong data = stringToHex(dataLst[j],isOk);
                    if (isOk)
                        memoryLine->datas.append((unsigned char)data);
                    else
                        memoryLine->datas.append(0);
                }
            } else {
                memoryLine->startAddress=0;
            }

        }
        newModel.append(memoryLine);
    }
    if (newModel.count()>0 && newModel.count()== mLines.count() &&
            newModel[0]->startAddress == mLines[0]->startAddress &&
            maxDataPerLine==mDataPerLine) {
        for (int i=0;i<newModel.count();i++) {
            PMemoryLine newLine = newModel[i];
            PMemoryLine oldLine = mLines[i];
            for (int j=0;j<newLine->datas.count();j++) {
                if (j>=oldLine->datas.count())
                    break;
                if (newLine->datas[j]!=oldLine->datas[j])
                    newLine->changedDatas.insert(j);
            }
        }
        mLines = newModel;
        emit dataChanged(createIndex(0,0),
                         createIndex(mLines.count()-1,mDataPerLine-1));
    } else {
        beginResetModel();
        if (maxDataPerLine>0)
            mDataPerLine=maxDataPerLine;
        mLines = newModel;
        endResetModel();
    }
    if (mLines.count()>0) {
        mStartAddress = mLines[0]->startAddress;
    } else {
        mStartAddress = 0;
    }
 }

int MemoryModel::rowCount(const QModelIndex &/*parent*/) const
{
    return mLines.count();
}

int MemoryModel::columnCount(const QModelIndex &/*parent*/) const
{
    return mDataPerLine+1;
}

QVariant MemoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (index.row()<0 || index.row()>=mLines.count())
        return QVariant();
    PMemoryLine line = mLines[index.row()];
    int col = index.column();
    if (col<0  || col>line->datas.count())
        return QVariant();
    if (role == Qt::DisplayRole) {
        if (col==line->datas.count()) {
            QString s;
            for (char ch : line->datas) {
                s += isAsciiPrint(ch) ? ch : '.';
            }
            return s;
        } else
            return QString("%1").arg(line->datas[col],2,16,QChar('0'));
    } else if (role == Qt::ToolTipRole) {
        if (col<line->datas.count()) {
            QString s =
                    tr("addr: %1").arg(line->startAddress+col,0,16)
                    +"<br/>"
                    +tr("dec: %1").arg(line->datas[col])
                    +"<br/>"
                    +tr("oct: %1").arg(line->datas[col],0,8)
                    +"<br/>"
                    +tr("bin: %1").arg(line->datas[col],8,2,QChar('0'))
                    +"<br/>";
            QString chVal;
            if (line->datas[col]==0) {
                chVal="\\0";
            } else if (line->datas[col]=='\n') {
                chVal="\\n";
            } else if (line->datas[col]=='\t') {
                chVal="\\t";
            } else if (line->datas[col]=='\r') {
                chVal="\\r";
            } else if (line->datas[col]>=' ' && line->datas[col]<127) {
                chVal=QChar(line->datas[col]);
            }
            if (!chVal.isEmpty()) {
                s+=tr("ascii: \'%1\'").arg(chVal)
                        +"<br/>";
            }
            return s;
        }
    }
    return QVariant();
}

QVariant MemoryModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical && role ==  Qt::DisplayRole) {
        if (section<0 || section>=mLines.count())
            return QVariant();
        PMemoryLine line = mLines[section];
        return QString("0x%1").arg(line->startAddress,0,16,QChar('0'));
    }
    return QVariant();
}

bool MemoryModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid())
        return false;
    if (index.row()<0 || index.row()>=mLines.count())
        return false;
    PMemoryLine line = mLines[index.row()];
    int col = index.column();
    if (col<0  || col>=line->datas.count())
        return false;
    if (role == Qt::EditRole && mStartAddress>0) {
        bool ok;
        unsigned char val = ("0x"+value.toString()).toUInt(&ok,16);
        if (!ok)
            return false;
        emit setMemoryData(mStartAddress+mDataPerLine*index.row()+col,val);
        return true;
    }
    return false;
}

Qt::ItemFlags MemoryModel::flags(const QModelIndex &/*index*/) const
{
    Qt::ItemFlags flags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (mStartAddress!=0)
        flags |= Qt::ItemIsEditable;
    return flags;
}

qulonglong MemoryModel::startAddress() const
{
    return mStartAddress;
}

void MemoryModel::reset()
{
    mStartAddress=0;
    mLines.clear();
}
