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
#include "../utils.h"
#include "../utils/parsearg.h"
#include "../mainwindow.h"
#include "../settings.h"
#include "../widgets/cpudialog.h"
#include "../systemconsts.h"
#include "../editormanager.h"
#include <QFile>
#include <QFileInfo>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QDebug>
#include <QDir>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include "../widgets/signalmessagedialog.h"
#include <QApplication>
#include <QRegularExpression>

Debugger::Debugger(QObject *parent) : QObject{parent},
    mForceUTF8{false},
    mDebugInfosUsingUTF8{false},
    mUseDebugServer{false},
    mDebuggerType{DebuggerType::GDB},
    mLastLoadtime{0},
    mProjectLastLoadtime{0},
    mInferiorHasBreakpoints{false},
    mClientMutex{}
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

    mClient = nullptr;
    mTarget = nullptr;
    mLeftPageIndexBackup = -1;

    connect(mWatchModel.get(), &WatchModel::fetchChildren,
            this, &Debugger::fetchVarChildren);

    setIsForProject(false);
}

Debugger::~Debugger()
{
    disconnect();
    // QObject::disconnect(mClient, nullptr, this, nullptr);
    // QObject::disconnect(mClient, nullptr, pMainWindow, nullptr);
    cleanUp();
}

bool Debugger::startClient(int compilerSetIndex,
                           const QString& inferior,
                           bool inferiorHasSymbols,
                           bool inferiorHasBreakpoints,
                           const QStringList& binDirs,
                           const QString& sourceFile)
{
    QMutexLocker locker{&mClientMutex};
    if (mClient!=nullptr)
        return false;
    mCurrentSourceFile = sourceFile;
    PCompilerSet compilerSet = pSettings->compilerSets().getSet(compilerSetIndex);
    if (!compilerSet) {
        compilerSet = pSettings->compilerSets().defaultSet();
    }
    if (!compilerSet) {
        QMessageBox::critical(pMainWindow,
                              tr("No compiler set"),
                              tr("No compiler set is configured.")+tr("Can't start debugging."));
        return false;
    }
    setForceUTF8(compilerSet->isDebuggerUsingUTF8());
    setDebugInfosUsingUTF8(compilerSet->isCompilerUsingUTF8());
    if (compilerSet->debugger().endsWith(LLDB_MI_PROGRAM))
        setDebuggerType(DebuggerType::LLDB_MI);
    else
        setDebuggerType(DebuggerType::GDB);
    // force to lldb-server if using lldb-mi, which creates new console but does not bind inferior’s stdio to the new console on Windows.
    setUseDebugServer(pSettings->debugger().useGDBServer() || mDebuggerType == DebuggerType::LLDB_MI);
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
        QMessageBox::critical(pMainWindow,
                              tr("Debugger not exists"),
                              tr("Can''t find debugger (gdb) in : \"%1\"").arg(debuggerPath)
                              +"<br />"
                              +tr("Please check the \"program\" page of compiler settings."));
        return false;
    }
    if (useDebugServer()) {
        if (!isTextAllAscii(compilerSet->debugServer())) {
            QMessageBox::critical(pMainWindow,
                                  tr("GDB Server path error"),
                                  tr("GDB Server's path \"%1\" contains non-ascii characters.")
                                  .arg(compilerSet->debugServer())
                                  + "<br />"
                                  + tr("This prevents it from executing."));
            return false;
        }
        if (!fileExists(compilerSet->debugServer())) {
            QMessageBox::critical(pMainWindow,
                                  tr("GDB Server not exists"),
                                  tr("Can''t find gdb server in : \"%1\"").arg(compilerSet->debugServer()));
            return false;
        }

    }
    mMemoryModel->reset();
    mWatchModel->resetAllVarInfos();
    if (useDebugServer()) {
        QStringList params;
        if (pSettings->executor().useParams())
            params = parseArgumentsWithoutVariables(pSettings->executor().params());
        mTarget = new DebugTarget(inferior,compilerSet->debugServer(),pSettings->debugger().GDBServerPort(),params);
        if (pSettings->executor().redirectInput())
            mTarget->setInputFile(pSettings->executor().inputFilename());
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
    connect(mClient, &QThread::finished,this,&Debugger::cleanUp);
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

        //skip C++ Stardard Libraries
        mClient->skipStandardLibraryFunctions();
    }

    sendAllBreakpointsToDebugger();
    pMainWindow->updateAppTitle();
    mInferiorHasBreakpoints = inferiorHasBreakpoints;
    return true;
}

void Debugger::runInferior()
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->runInferior(mInferiorHasBreakpoints);
}

void Debugger::stop() {
    QMutexLocker locker{&mClientMutex};
    if (mTarget)
        mTarget->stopDebug();
    if (mClient) {
        mClient->stopDebug();
    }
}

void Debugger::cleanUp()
{
    QMutexLocker locker{&mClientMutex};
    if (mClient) {
        //stop gdb server (if runs)
        if (mTarget) {
            mTarget->stopDebug();
            mTarget->deleteLater();
            mTarget = nullptr;
        }

        //stop debugger
        mClient->stopDebug();
        mClient->deleteLater();
        mClient = nullptr;

        mCurrentSourceFile="";

        mBacktraceModel->clear();

        mWatchModel->clearAllVarInfos();

        mBreakpointModel->invalidateAllBreakpointNumbers();
    }
    emit debugFinished();
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
    QMutexLocker locker{&mClientMutex};
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

bool Debugger::commandRunning() const
{
    QMutexLocker locker{&mClientMutex};
    if (mClient) {
        return mClient->commandRunning();
    }
    return false;
}

bool Debugger::inferiorRunning()
{
    QMutexLocker locker{&mClientMutex};
    if (mClient) {
        return mClient->inferiorRunning();
    }
    return false;
}

void Debugger::interrupt()
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->interrupt();
}

void Debugger::stepOver()
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->stepOver();
}

void Debugger::stepInto()
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->stepInto();
}

void Debugger::stepOut()
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->stepOut();
}

void Debugger::runTo(const QString &filename, int line)
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->runTo(filename, line);
}

void Debugger::resume()
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->resume();
}

void Debugger::stepOverInstruction()
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->stepOverInstruction();
}

void Debugger::stepIntoInstruction()
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->stepIntoInstruction();
}

void Debugger::runClientCommand(const QString &command, const QString &params, DebugCommandSource source)
{
    QMutexLocker locker{&mClientMutex};
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
    QMutexLocker locker{&mClientMutex};
    if (!mClient) {
        mBreakpointModel->setIsForProject(newIsForProject);
        mWatchModel->setIsForProject(newIsForProject);
    }
}

void Debugger::clearForProject()
{
    mBreakpointModel->clear(true);
    mWatchModel->clear(true);
}

void Debugger::addBreakpoint(int line, const QString &filename, bool forProject)
{
    QMutexLocker locker{&mClientMutex};
    PBreakpoint bp=std::make_shared<Breakpoint>();
    bp->number = -1;
    bp->line = line;
    bp->filename = filename;
    bp->condition = "";
    bp->enabled = true;
    bp->breakpointType = BreakpointType::Breakpoint;
    bp->timestamp = QDateTime::currentMSecsSinceEpoch();
    mBreakpointModel->addBreakpoint(bp,forProject);
    if (!mClient) {
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

void Debugger::setBreakPointCondition(int index, const QString &condition, bool forProject)
{
    QMutexLocker locker{&mClientMutex};
    PBreakpoint breakpoint=mBreakpointModel->setBreakPointCondition(index,condition, forProject);
    if (mClient)
        mClient->setBreakpointCondition(breakpoint);
}

void Debugger::sendAllBreakpointsToDebugger()
{
    for (const PBreakpoint &breakpoint:mBreakpointModel->breakpoints(mBreakpointModel->isForProject())) {
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
    QMutexLocker locker{&mClientMutex};
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
        if (executing() && !var->expression.isEmpty())
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
    QMutexLocker locker{&mClientMutex};
    if (mClient) {
        sendAllWatchVarsToDebugger();
        if (mDebuggerType==DebuggerType::LLDB_MI) {
            for (const PWatchVar &var:mWatchModel->watchVars()) {
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
    QMutexLocker locker{&mClientMutex};
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
    for (const PWatchVar &var:mWatchModel->watchVars()) {
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
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->readMemory(startAddress, rows, cols);
}

void Debugger::evalExpression(const QString &expression)
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->evalExpression(expression);
}

void Debugger::selectFrame(PTrace trace)
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->selectFrame(trace);
}

void Debugger::refreshFrame()
{
    QMutexLocker locker{&mClientMutex};
    if (mClient) {
        mClient->refreshFrame();
    }
}

void Debugger::refreshStackVariables()
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->refreshStackVariables();
}

void Debugger::refreshRegisters()
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->refreshRegisters();
}

void Debugger::disassembleCurrentFrame(bool blendMode)
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->disassembleCurrentFrame(blendMode);
}

void Debugger::setDisassemblyLanguage(bool isIntel)
{
    QMutexLocker locker{&mClientMutex};
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
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->addWatch(var->expression);
}

void Debugger::sendRemoveWatchCommand(PWatchVar var)
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->removeWatch(var);
}

void Debugger::sendBreakpointCommand(PBreakpoint breakpoint)
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->addBreakpoint(breakpoint);
}

void Debugger::sendClearBreakpointCommand(int index, bool forProject)
{
    sendClearBreakpointCommand(mBreakpointModel->breakpoints(forProject)[index]);
}

void Debugger::sendClearBreakpointCommand(PBreakpoint breakpoint)
{
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->removeBreakpoint(breakpoint);
}

void Debugger::save(const QString &filename, const QString& projectFolder)
{
    bool forProject=!projectFolder.isEmpty();
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
        } else {
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
    QMutexLocker locker{&mClientMutex};
    if (mClient)
        mClient->writeMemory(address, data);
    refreshAll();
}

void Debugger::setWatchVarValue(const QString &name, const QString &value)
{
    QMutexLocker locker{&mClientMutex};
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

bool Debugger::executing()
{
    QMutexLocker locker{&mClientMutex};
    return (mClient!=nullptr);
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

    QProcess process;
    process.setProgram(cmd);
    process.setArguments(arguments);
    process.setProcessChannelMode(QProcess::MergedChannels);
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
    process.setProcessEnvironment(env);
    process.setWorkingDirectory(workingDir);

#ifdef Q_OS_WIN
    process.setCreateProcessArgumentsModifier([this](QProcess::CreateProcessArguments * args){
        if (!programIsWin32GuiApp(mInferior)) {
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

    connect(&process, &QProcess::errorOccurred,
                    [&](){
                        mErrorOccured= true;
                    });
    process.start();
    process.waitForStarted(5000);
    mStartSemaphore.release(1);
    if (process.state()==QProcess::Running && !mInputFile.isEmpty()) {
        process.write(readFileToByteArray(mInputFile));
        process.waitForFinished(0);
    }
    bool writeChannelClosed = false;
    while (true) {
        if (process.bytesToWrite()==0 && !writeChannelClosed) {
            writeChannelClosed = true;
            process.closeWriteChannel();
        }
        process.waitForFinished(1);
        if (process.state()!=QProcess::Running) {
            break;
        }
        if (mStop) {
            process.terminate();
            process.kill();
            break;
        }
        if (mErrorOccured)
            break;
        msleep(1);
    }
    if (mErrorOccured) {
        emit processFailed(process.error());
    }
}

