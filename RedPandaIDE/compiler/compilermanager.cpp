#include "compilermanager.h"
#include "filecompiler.h"
#include <QDebug>
#include "../mainwindow.h"

CompilerManager::CompilerManager(QObject *parent) : QObject(parent)
{
    mCompiler = nullptr;
    mBackgroundSyntaxChecker = nullptr;
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
    {
        QMutexLocker locker(&compileMutex);
        if (mCompiler!=nullptr) {
            return;
        }
        mCompiler = new FileCompiler(filename,encoding,silent,onlyCheckSyntax);
        connect(mCompiler, &Compiler::compileFinished, this ,&CompilerManager::onCompileFinished);
        connect(mCompiler, &Compiler::compileOutput, pMainWindow, &MainWindow::onCompileLog);
        connect(mCompiler, &Compiler::compileError, pMainWindow, &MainWindow::onCompileError);
        mCompiler->start();
    }
}

void CompilerManager::onCompileFinished()
{
    QMutexLocker locker(&compileMutex);
    qDebug() << " Compile Finished";

    mCompiler=nullptr;
}

