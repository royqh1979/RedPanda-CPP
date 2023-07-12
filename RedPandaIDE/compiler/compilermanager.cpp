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
#include "compilermanager.h"
#include "filecompiler.h"
#include "stdincompiler.h"
#include "../mainwindow.h"
#include "executablerunner.h"
#include "ojproblemcasesrunner.h"
#include "utils.h"
#include "../systemconsts.h"
#include "../settings.h"
#include <QMessageBox>
#include <QUuid>
#include "projectcompiler.h"

enum RunProgramFlag {
    RPF_PAUSE_CONSOLE =     0x0001,
    RPF_REDIRECT_INPUT =    0x0002
};

CompilerManager::CompilerManager(QObject *parent) : QObject(parent),
#if QT_VERSION >= QT_VERSION_CHECK(5, 15, 0)
    mCompileMutex(),
    mBackgroundSyntaxCheckMutex(),
    mRunnerMutex()
#else
    mCompileMutex(QMutex::Recursive),
    mBackgroundSyntaxCheckMutex(QMutex::Recursive),
    mRunnerMutex(QMutex::Recursive)
#endif
{
    mCompiler = nullptr;
    mBackgroundSyntaxChecker = nullptr;
    mRunner = nullptr;
    mSyntaxCheckErrorCount = 0;
    mSyntaxCheckIssueCount = 0;
    mCompileErrorCount = 0;
    mCompileIssueCount = 0;
    mSyntaxCheckErrorCount = 0;
}

bool CompilerManager::compiling()
{
    QMutexLocker locker(&mCompileMutex);
    return mCompiler!=nullptr;
}

bool CompilerManager::backgroundSyntaxChecking()
{
    QMutexLocker locker(&mBackgroundSyntaxCheckMutex);
    return mBackgroundSyntaxChecker!=nullptr;
}

bool CompilerManager::running()
{
    QMutexLocker locker(&mRunnerMutex);
    return (mRunner!=nullptr && !mRunner->pausing());
}

void CompilerManager::compile(const QString& filename, const QByteArray& encoding, bool rebuild, CppCompileType compileType)
{
    if (!pSettings->compilerSets().defaultSet()) {
        QMessageBox::critical(pMainWindow,
                              tr("No compiler set"),
                              tr("No compiler set is configured.")+tr("Can't start debugging."));
        return;
    }
    {
        QMutexLocker locker(&mCompileMutex);
        if (mCompiler!=nullptr) {
            return;
        }
        mCompileErrorCount = 0;
        mCompileIssueCount = 0;
        //deleted when thread finished
        mCompiler = new FileCompiler(filename,encoding,compileType,false,false);
        mCompiler->setRebuild(rebuild);
        connect(mCompiler, &Compiler::finished, mCompiler, &QObject::deleteLater);
        connect(mCompiler, &Compiler::compileFinished, this, &CompilerManager::onCompileFinished);
        connect(mCompiler, &Compiler::compileIssue, this, &CompilerManager::onCompileIssue);
        connect(mCompiler, &Compiler::compileStarted, pMainWindow, &MainWindow::onCompileStarted);
        connect(mCompiler, &Compiler::compileStarted, pMainWindow, &MainWindow::clearToolsOutput);

        connect(mCompiler, &Compiler::compileOutput, pMainWindow, &MainWindow::logToolsOutput);
        connect(mCompiler, &Compiler::compileIssue, pMainWindow, &MainWindow::onCompileIssue);
        connect(mCompiler, &Compiler::compileErrorOccured, pMainWindow, &MainWindow::onCompileErrorOccured);
        mCompiler->start();
    }
}

void CompilerManager::compileProject(std::shared_ptr<Project> project, bool rebuild)
{
    if (!pSettings->compilerSets().defaultSet()) {
        QMessageBox::critical(pMainWindow,
                              tr("No compiler set"),
                              tr("No compiler set is configured.")+tr("Can't start debugging."));
        return;
    }
    {
        QMutexLocker locker(&mCompileMutex);
        if (mCompiler!=nullptr) {
            return;
        }
        mCompileErrorCount = 0;
        mCompileIssueCount = 0;
        //deleted when thread finished
        mCompiler = new ProjectCompiler(project,false,false);
        mCompiler->setRebuild(rebuild);
        connect(mCompiler, &Compiler::finished, mCompiler, &QObject::deleteLater);
        connect(mCompiler, &Compiler::compileFinished, this, &CompilerManager::onCompileFinished);

        connect(mCompiler, &Compiler::compileIssue, this, &CompilerManager::onCompileIssue);
        connect(mCompiler, &Compiler::compileStarted, pMainWindow, &MainWindow::onProjectCompileStarted);
        connect(mCompiler, &Compiler::compileStarted, pMainWindow, &MainWindow::clearToolsOutput);

        connect(mCompiler, &Compiler::compileOutput, pMainWindow, &MainWindow::logToolsOutput);
        connect(mCompiler, &Compiler::compileIssue, pMainWindow, &MainWindow::onCompileIssue);
        connect(mCompiler, &Compiler::compileErrorOccured, pMainWindow, &MainWindow::onCompileErrorOccured);
        mCompiler->start();
    }
}

void CompilerManager::cleanProject(std::shared_ptr<Project> project)
{
    if (!pSettings->compilerSets().defaultSet()) {
        QMessageBox::critical(pMainWindow,
                              tr("No compiler set"),
                              tr("No compiler set is configured.")+tr("Can't start debugging."));
        return;
    }
    {
        QMutexLocker locker(&mCompileMutex);
        if (mCompiler!=nullptr) {
            return;
        }
        mCompileErrorCount = 0;
        mCompileIssueCount = 0;
        //deleted when thread finished
        ProjectCompiler* compiler = new ProjectCompiler(project,false,false);
        compiler->setOnlyClean(true);
        mCompiler = compiler;
        mCompiler->setRebuild(false);
        connect(mCompiler, &Compiler::finished, mCompiler, &QObject::deleteLater);
        connect(mCompiler, &Compiler::compileFinished, this, &CompilerManager::onCompileFinished);

        connect(mCompiler, &Compiler::compileIssue, this, &CompilerManager::onCompileIssue);
        connect(mCompiler, &Compiler::compileStarted, pMainWindow, &MainWindow::onProjectCompileStarted);
        connect(mCompiler, &Compiler::compileStarted, pMainWindow, &MainWindow::clearToolsOutput);

        connect(mCompiler, &Compiler::compileOutput, pMainWindow, &MainWindow::logToolsOutput);
        connect(mCompiler, &Compiler::compileIssue, pMainWindow, &MainWindow::onCompileIssue);
        connect(mCompiler, &Compiler::compileErrorOccured, pMainWindow, &MainWindow::onCompileErrorOccured);
        mCompiler->start();
    }
}

void CompilerManager::buildProjectMakefile(std::shared_ptr<Project> project)
{
    if (!pSettings->compilerSets().defaultSet()) {
        QMessageBox::critical(pMainWindow,
                              tr("No compiler set"),
                              tr("No compiler set is configured.")+tr("Can't start debugging."));
        return;
    }
    {
        QMutexLocker locker(&mCompileMutex);
        if (mCompiler!=nullptr) {
            return;
        }
        ProjectCompiler compiler(project,false,false);
        compiler.buildMakeFile();
    }

}

void CompilerManager::checkSyntax(const QString &filename, const QByteArray& encoding, const QString &content, std::shared_ptr<Project> project)
{
    if (!pSettings->compilerSets().defaultSet()) {
        QMessageBox::critical(pMainWindow,
                              tr("No compiler set"),
                              tr("No compiler set is configured.")+tr("Can't start debugging."));
        return;
    }
    {
        QMutexLocker locker(&mBackgroundSyntaxCheckMutex);
        if (mBackgroundSyntaxChecker!=nullptr) {
            return;
        }

        mSyntaxCheckErrorCount = 0;
        mSyntaxCheckIssueCount = 0;

        //deleted when thread finished
        mBackgroundSyntaxChecker = new StdinCompiler(filename,encoding, content,true,true);
        mBackgroundSyntaxChecker->setProject(project);
        connect(mBackgroundSyntaxChecker, &Compiler::finished, mBackgroundSyntaxChecker, &QThread::deleteLater);
        connect(mBackgroundSyntaxChecker, &Compiler::compileIssue, this, &CompilerManager::onSyntaxCheckIssue);
        connect(mBackgroundSyntaxChecker, &Compiler::compileStarted, pMainWindow, &MainWindow::onSyntaxCheckStarted);
        connect(mBackgroundSyntaxChecker, &Compiler::compileFinished, this, &CompilerManager::onSyntaxCheckFinished);
        //connect(mBackgroundSyntaxChecker, &Compiler::compileOutput, pMainWindow, &MainWindow::logToolsOutput);
        connect(mBackgroundSyntaxChecker, &Compiler::compileIssue, pMainWindow, &MainWindow::onCompileIssue);
        connect(mBackgroundSyntaxChecker, &Compiler::compileErrorOccured, pMainWindow, &MainWindow::onCompileErrorOccured);
        mBackgroundSyntaxChecker->start();
    }
}

void CompilerManager::run(
        const QString &filename,
        const QString &arguments,
        const QString &workDir,
        const QStringList& binDirs)
{
    QMutexLocker locker(&mRunnerMutex);
    if (mRunner!=nullptr && !mRunner->pausing()) {
        return;
    }
    QString redirectInputFilename;
    bool redirectInput=false;
    if (pSettings->executor().redirectInput()
            && !pSettings->executor().inputFilename().isEmpty()) {
        redirectInput =true;
        redirectInputFilename = pSettings->executor().inputFilename();
    }
    ExecutableRunner * execRunner;
    if (programHasConsole(filename)) {
        int consoleFlag=0;
        if (redirectInput)
            consoleFlag |= RPF_REDIRECT_INPUT;
        if (pSettings->executor().pauseConsole())
            consoleFlag |= RPF_PAUSE_CONSOLE;
#ifdef Q_OS_WIN
        if (consoleFlag!=0) {
            QString sharedMemoryId = QUuid::createUuid().toString();
            QString newArguments = QString(" %1 %2 \"%3\" %4")
                    .arg(consoleFlag)
                    .arg(sharedMemoryId,localizePath(filename)).arg(arguments);

            //delete when thread finished
            execRunner = new ExecutableRunner(includeTrailingPathDelimiter(pSettings->dirs().appDir())+CONSOLE_PAUSER,newArguments,workDir);
            execRunner->setShareMemoryId(sharedMemoryId);
        } else {
            //delete when thread finished
            execRunner = new ExecutableRunner(filename,arguments,workDir);
        }
#else
        QString newArguments;
        QString sharedMemoryId = "/r"+QUuid::createUuid().toString(QUuid::StringFormat::Id128);
        if (consoleFlag!=0) {
            QString consolePauserPath=includeTrailingPathDelimiter(pSettings->dirs().appLibexecDir())+"consolepauser";
            if (!fileExists(consolePauserPath)) {
                QMessageBox::critical(pMainWindow,
                                         tr("Can't find Console Pauser"),
                                         tr("Console Pauser \"%1\" doesn't exists!")
                                         .arg(consolePauserPath));
                return;

            }
            if (redirectInput) {
                newArguments = QString(" -e \"%1\" %2 %3 \"%4\" \"%5\" %6")
                        .arg(consolePauserPath)
                        .arg(consoleFlag)
                        .arg(sharedMemoryId)
                        .arg(escapeSpacesInString(redirectInputFilename))
                        .arg(localizePath(escapeSpacesInString(filename)))
                        .arg(arguments);
            } else {
                newArguments = QString(" -e \"%1\" %2 %3 \"%4\" %5")
                    .arg(consolePauserPath)
                    .arg(consoleFlag)
                    .arg(sharedMemoryId,localizePath(escapeSpacesInString(filename))).arg(arguments);
            }
        } else {
            newArguments = QString(" -e \"%1\" %2")
                .arg(localizePath(escapeSpacesInString(filename))).arg(arguments);
        }
        execRunner = new ExecutableRunner(pSettings->environment().terminalPathForExec(),newArguments,workDir);
        execRunner->setShareMemoryId(sharedMemoryId);
#endif
        execRunner->setStartConsole(true);
    } else {
        //delete when thread finished
        execRunner = new ExecutableRunner(filename,arguments,workDir);
    }    
    if (redirectInput) {
        execRunner->setRedirectInput(true);
        execRunner->setRedirectInputFilename(redirectInputFilename);
    }
    execRunner->addBinDirs(binDirs);

    execRunner->addBinDir(pSettings->dirs().appDir());

    mRunner = execRunner;

    connect(mRunner, &Runner::finished, this ,&CompilerManager::onRunnerTerminated);
    connect(mRunner, &Runner::finished, mRunner ,&Runner::deleteLater);
    connect(mRunner, &Runner::finished, pMainWindow ,&MainWindow::onRunFinished);
    connect(mRunner, &Runner::pausingForFinish, pMainWindow ,&MainWindow::onRunPausingForFinish);
    connect(mRunner, &Runner::pausingForFinish, this ,&CompilerManager::onRunnerPausing);
    connect(mRunner, &Runner::runErrorOccurred, pMainWindow ,&MainWindow::onRunErrorOccured);
    mRunner->start();
}


void CompilerManager::runProblem(const QString &filename, const QString &arguments, const QString &workDir, POJProblemCase problemCase,
                                 const POJProblem& problem
                                 )
{
    QMutexLocker locker(&mRunnerMutex);
    doRunProblem(filename, arguments, workDir, QVector<POJProblemCase> {problemCase}, problem);

}

void CompilerManager::runProblem(const QString &filename, const QString &arguments, const QString &workDir, const QVector<POJProblemCase>& problemCases,
                                 const POJProblem& problem
                                 )
{
    QMutexLocker locker(&mRunnerMutex);
    doRunProblem(filename, arguments, workDir, problemCases, problem);
}
void CompilerManager::doRunProblem(const QString &filename, const QString &arguments, const QString &workDir, const QVector<POJProblemCase>& problemCases,
                                   const POJProblem& problem)
{
    if (mRunner!=nullptr) {
        return;
    }
    OJProblemCasesRunner * execRunner = new OJProblemCasesRunner(filename,arguments,workDir,problemCases);
    mRunner = execRunner;
    if (pSettings->executor().enableCaseLimit()) {
        execRunner->setExecTimeout(pSettings->executor().caseTimeout());
        execRunner->setMemoryLimit(pSettings->executor().caseMemoryLimit()*1024); //convert kb to bytes
    }
    size_t timeLimit = problem->getTimeLimit();
    size_t memoryLimit = problem->getMemoryLimit();
    if (timeLimit>0)
        execRunner->setExecTimeout(timeLimit);
    if (memoryLimit)
        execRunner->setMemoryLimit(memoryLimit);
    connect(mRunner, &Runner::finished, this ,&CompilerManager::onRunnerTerminated);
    connect(mRunner, &Runner::finished, mRunner ,&Runner::deleteLater);
    connect(mRunner, &Runner::finished, pMainWindow ,&MainWindow::onRunProblemFinished);
    connect(mRunner, &Runner::runErrorOccurred, pMainWindow ,&MainWindow::onRunErrorOccured);
    connect(execRunner, &OJProblemCasesRunner::caseStarted, pMainWindow, &MainWindow::onOJProblemCaseStarted);
    connect(execRunner, &OJProblemCasesRunner::caseFinished, pMainWindow, &MainWindow::onOJProblemCaseFinished);
    connect(execRunner, &OJProblemCasesRunner::newOutputGetted, pMainWindow, &MainWindow::onOJProblemCaseNewOutputGetted);
    connect(execRunner, &OJProblemCasesRunner::resetOutput, pMainWindow, &MainWindow::onOJProblemCaseResetOutput);
    if (pSettings->executor().redirectStderrToToolLog()) {
        connect(execRunner, &OJProblemCasesRunner::logStderrOutput, pMainWindow, &MainWindow::logToolsOutput);
    }
    mRunner->start();
}

void CompilerManager::stopRun()
{
    QMutexLocker locker(&mRunnerMutex);
    if (mRunner!=nullptr) {
        mRunner->stop();
        disconnect(mRunner, &Runner::finished, this ,&CompilerManager::onRunnerTerminated);
        mRunner=nullptr;
    }
}

void CompilerManager::stopAllRunners()
{
    emit signalStopAllRunners();
}

void CompilerManager::stopPausing()
{
    QMutexLocker locker(&mRunnerMutex);
    if (mRunner!=nullptr && mRunner->pausing()) {
        disconnect(mRunner, &Runner::finished, this ,&CompilerManager::onRunnerTerminated);
        mRunner->stop();
        mRunner=nullptr;
    }
}

void CompilerManager::stopCompile()
{
    QMutexLocker locker(&mCompileMutex);
    if (mCompiler!=nullptr)
        mCompiler->stopCompile();
}

void CompilerManager::stopCheckSyntax()
{
    QMutexLocker locker(&mBackgroundSyntaxCheckMutex);
    if (mBackgroundSyntaxChecker!=nullptr)
        mBackgroundSyntaxChecker->stopCompile();
}

bool CompilerManager::canCompile(const QString &)
{
    return !compiling();
}

void CompilerManager::onCompileFinished(QString filename)
{
    QMutexLocker locker(&mCompileMutex);
    mCompiler=nullptr;
    pMainWindow->onCompileFinished(filename,false);
}

void CompilerManager::onRunnerTerminated()
{
    QMutexLocker locker(&mRunnerMutex);
    mRunner=nullptr;
}

void CompilerManager::onRunnerPausing()
{
    QMutexLocker locker(&mRunnerMutex);
    disconnect(mRunner, &Runner::finished, this ,&CompilerManager::onRunnerTerminated);
    disconnect(mRunner, &Runner::finished, pMainWindow ,&MainWindow::onRunFinished);
    disconnect(mRunner, &Runner::runErrorOccurred, pMainWindow ,&MainWindow::onRunErrorOccured);
    connect(this, &CompilerManager::signalStopAllRunners, mRunner, &Runner::stop);
    mRunner=nullptr;
}

void CompilerManager::onCompileIssue(PCompileIssue issue)
{
    if (issue->type == CompileIssueType::Error)
        mCompileErrorCount++;
    mCompileIssueCount++;
}

void CompilerManager::onSyntaxCheckFinished(QString filename)
{
    QMutexLocker locker(&mBackgroundSyntaxCheckMutex);
    mBackgroundSyntaxChecker=nullptr;
    pMainWindow->onCompileFinished(filename, true);
}

void CompilerManager::onSyntaxCheckIssue(PCompileIssue issue)
{
    if (issue->type == CompileIssueType::Error)
        mSyntaxCheckErrorCount++;
    if (issue->type == CompileIssueType::Error ||
            issue->type == CompileIssueType::Warning)
        mSyntaxCheckIssueCount++;
}

int CompilerManager::syntaxCheckIssueCount() const
{
    return mSyntaxCheckIssueCount;
}

int CompilerManager::compileIssueCount() const
{
    return mCompileIssueCount;
}

int CompilerManager::syntaxCheckErrorCount() const
{
    return mSyntaxCheckErrorCount;
}

int CompilerManager::compileErrorCount() const
{
    return mCompileErrorCount;
}

CompileError::CompileError(const QString &reason):BaseError(reason)
{

}

