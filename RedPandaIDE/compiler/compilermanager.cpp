#include "compilermanager.h"
#include "filecompiler.h"
#include <QDebug>
#include "../mainwindow.h"
#include "executablerunner.h"
#include "utils.h"

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
    return mCompiler!=nullptr;
}

bool CompilerManager::backgroundSyntaxChecking()
{
    return mBackgroundSyntaxChecker!=nullptr;
}

void CompilerManager::compile(const QString& filename, const QByteArray& encoding, bool silent, bool onlyCheckSyntax)
{
    QMutexLocker locker(&compileMutex);
    if (mCompiler!=nullptr) {
        return;
    }
    mCompileErrorCount = 0;
    mCompiler = new FileCompiler(filename,encoding,silent,onlyCheckSyntax);
    connect(mCompiler, &Compiler::compileFinished, this ,&CompilerManager::onCompileFinished);
    connect(mCompiler, &Compiler::compileIssue, this, &CompilerManager::onCompileIssue);
    connect(mCompiler, &Compiler::compileFinished, pMainWindow, &MainWindow::onCompileFinished);
    connect(mCompiler, &Compiler::compileOutput, pMainWindow, &MainWindow::onCompileLog);
    connect(mCompiler, &Compiler::compileIssue, pMainWindow, &MainWindow::onCompileIssue);
    connect(mCompiler, &Compiler::compileErrorOccured, pMainWindow, &MainWindow::onCompileErrorOccured);
    mCompiler->start();
}

void CompilerManager::run(const QString &filename, const QString &arguments, const QString &workDir)
{
    QMutexLocker locker(&runnerMutex);
    if (mRunner!=nullptr) {
        return;
    }
    if (programHasConsole(filename)) {
        QString newArguments = QString(" 0 \"%1\" %2").arg(toLocalPath(filename)).arg(arguments);
        mRunner = new ExecutableRunner(includeTrailingPathDelimiter(pSettings->dirs().app())+"ConsolePauser.exe",newArguments,workDir);
    } else {
        mRunner = new ExecutableRunner(filename,arguments,workDir);
    }
    connect(mRunner, &ExecutableRunner::terminated, this ,&CompilerManager::onRunnerTerminated);
    mRunner->start();
}

bool CompilerManager::canCompile(const QString &filename)
{
    return !compiling();
}

void CompilerManager::onCompileFinished()
{
    QMutexLocker locker(&compileMutex);
    delete mCompiler;
    mCompiler=nullptr;
}

void CompilerManager::onRunnerTerminated()
{
    QMutexLocker locker(&runnerMutex);
    qDebug() << "Runner Terminated";
    mRunner=nullptr;
}

void CompilerManager::onCompileIssue(PCompileIssue)
{
    mCompileErrorCount ++;
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

