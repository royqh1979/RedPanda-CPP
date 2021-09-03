#include "compilermanager.h"
#include "filecompiler.h"
#include "stdincompiler.h"
#include <QDebug>
#include "../mainwindow.h"
#include "executablerunner.h"
#include "utils.h"
#include "../settings.h"
#include <QMessageBox>

CompilerManager::CompilerManager(QObject *parent) : QObject(parent)
{
    mCompiler = nullptr;
    mBackgroundSyntaxChecker = nullptr;
    mRunner = nullptr;
    mSyntaxCheckErrorCount = 0;
    mCompileErrorCount = 0;
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
    return mRunner!=nullptr;
}

void CompilerManager::compile(const QString& filename, const QByteArray& encoding, bool rebuild, bool silent, bool onlyCheckSyntax)
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
        mCompiler = new FileCompiler(filename,encoding,silent,onlyCheckSyntax);
        mCompiler->setRebuild(rebuild);
        connect(mCompiler, &Compiler::compileFinished, this ,&CompilerManager::onCompileFinished);
        connect(mCompiler, &Compiler::compileIssue, this, &CompilerManager::onCompileIssue);
        connect(mCompiler, &Compiler::compileFinished, pMainWindow, &MainWindow::onCompileFinished);
        connect(mCompiler, &Compiler::compileOutput, pMainWindow, &MainWindow::onCompileLog);
        connect(mCompiler, &Compiler::compileIssue, pMainWindow, &MainWindow::onCompileIssue);
        connect(mCompiler, &Compiler::compileErrorOccured, pMainWindow, &MainWindow::onCompileErrorOccured);
        mCompiler->start();
    }
}

void CompilerManager::checkSyntax(const QString &filename, const QString &content, bool isAscii)
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
        mBackgroundSyntaxChecker = new StdinCompiler(filename,content,isAscii,true,true);
        connect(mBackgroundSyntaxChecker, &Compiler::compileFinished, this ,&CompilerManager::onSyntaxCheckFinished);
        connect(mBackgroundSyntaxChecker, &Compiler::compileIssue, this, &CompilerManager::onSyntaxCheckIssue);
        connect(mBackgroundSyntaxChecker, &Compiler::compileFinished, pMainWindow, &MainWindow::onCompileFinished);
        connect(mBackgroundSyntaxChecker, &Compiler::compileOutput, pMainWindow, &MainWindow::onCompileLog);
        connect(mBackgroundSyntaxChecker, &Compiler::compileIssue, pMainWindow, &MainWindow::onCompileIssue);
        connect(mBackgroundSyntaxChecker, &Compiler::compileErrorOccured, pMainWindow, &MainWindow::onCompileErrorOccured);
        mBackgroundSyntaxChecker->start();
    }
}

void CompilerManager::run(const QString &filename, const QString &arguments, const QString &workDir)
{
    QMutexLocker locker(&mRunnerMutex);
    if (mRunner!=nullptr) {
        return;
    }
    if (pSettings->executor().pauseConsole() && programHasConsole(filename)) {
        QString newArguments = QString(" 0 \"%1\" %2").arg(toLocalPath(filename)).arg(arguments);
        mRunner = new ExecutableRunner(includeTrailingPathDelimiter(pSettings->dirs().app())+"ConsolePauser.exe",newArguments,workDir);
    } else {
        mRunner = new ExecutableRunner(filename,arguments,workDir);
    }
    connect(mRunner, &ExecutableRunner::finished, this ,&CompilerManager::onRunnerTerminated);
    connect(mRunner, &ExecutableRunner::finished, pMainWindow ,&MainWindow::onRunFinished);
    connect(mRunner, &ExecutableRunner::runErrorOccurred, pMainWindow ,&MainWindow::onRunErrorOccured);
    mRunner->start();
}

void CompilerManager::stopRun()
{
    QMutexLocker locker(&mRunnerMutex);
    if (mRunner!=nullptr)
        mRunner->stop();
}

void CompilerManager::stopCompile()
{
    QMutexLocker locker(&mCompileMutex);
    if (mCompiler!=nullptr)
        mCompiler->stopCompile();
}

bool CompilerManager::canCompile(const QString &filename)
{
    return !compiling();
}

void CompilerManager::onCompileFinished()
{
    QMutexLocker locker(&mCompileMutex);
    delete mCompiler;
    mCompiler=nullptr;
}

void CompilerManager::onRunnerTerminated()
{
    QMutexLocker locker(&mRunnerMutex);
    ExecutableRunner* p=mRunner;
    mRunner=nullptr;
    p->deleteLater();
}

void CompilerManager::onCompileIssue(PCompileIssue)
{
    mCompileErrorCount ++;
}

void CompilerManager::onSyntaxCheckFinished()
{
    QMutexLocker locker(&mBackgroundSyntaxCheckMutex);
    delete mBackgroundSyntaxChecker;
    mBackgroundSyntaxChecker=nullptr;
}

void CompilerManager::onSyntaxCheckIssue(PCompileIssue)
{
    mSyntaxCheckErrorCount++;
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

