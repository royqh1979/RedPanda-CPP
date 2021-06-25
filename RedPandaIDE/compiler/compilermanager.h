#ifndef COMPILERMANAGER_H
#define COMPILERMANAGER_H

#include <QObject>
#include <QMutex>
#include "../utils.h"
#include "../common.h"

class ExecutableRunner;
class Compiler;
class CompilerManager : public QObject
{
    Q_OBJECT
public:
    explicit CompilerManager(QObject *parent = nullptr);

    bool compiling();
    bool backgroundSyntaxChecking();
    bool running();

    void compile(const QString& filename, const QByteArray& encoding, bool rebuild, bool silent=false,bool onlyCheckSyntax=false);
    void checkSyntax(const QString&filename, const QString& content);
    void run(const QString& filename, const QString& arguments, const QString& workDir);
    void stopRun();
    void stopCompile();
    bool canCompile(const QString& filename);
    int compileErrorCount() const;

    int syntaxCheckErrorCount() const;

private slots:
    void onRunnerTerminated();
    void onCompileFinished();
    void onCompileIssue(PCompileIssue issue);
    void onSyntaxCheckFinished();
    void onSyntaxCheckIssue(PCompileIssue issue);

private:
    Compiler* mCompiler;
    int mCompileErrorCount;
    int mSyntaxCheckErrorCount;
    Compiler* mBackgroundSyntaxChecker;
    ExecutableRunner* mRunner;
    QMutex mCompileMutex;
    QMutex mBackgroundSyntaxCheckMutex;
    QMutex mRunnerMutex;
};

class CompileError : public BaseError {
public:
    explicit CompileError(const QString& reason);
};

#endif // COMPILERMANAGER_H
