#ifndef COMPILERMANAGER_H
#define COMPILERMANAGER_H

#include <QObject>
#include <QMutex>
#include "../utils.h"
#include "../common.h"

class Runner;
class Compiler;
class Project;
class OJProblemCase;
using POJProblemCase = std::shared_ptr<OJProblemCase>;
class CompilerManager : public QObject
{
    Q_OBJECT
public:
    explicit CompilerManager(QObject *parent = nullptr);

    bool compiling();
    bool backgroundSyntaxChecking();
    bool running();

    void compile(const QString& filename, const QByteArray& encoding, bool rebuild, bool silent=false,bool onlyCheckSyntax=false);
    void compileProject(std::shared_ptr<Project> project, bool rebuild, bool silent=false,bool onlyCheckSyntax=false);
    void cleanProject(std::shared_ptr<Project> project);
    void buildProjectMakefile(std::shared_ptr<Project> project);
    void checkSyntax(const QString&filename, const QString& content, bool isAscii, std::shared_ptr<Project> project);
    void run(const QString& filename, const QString& arguments, const QString& workDir);
    void runProblem(const QString& filename, const QString& arguments, const QString& workDir, POJProblemCase problemCase);
    void runProblem(const QString& filename, const QString& arguments, const QString& workDir, QVector<POJProblemCase> problemCases);
    void stopRun();
    void stopCompile();
    void stopCheckSyntax();
    bool canCompile(const QString& filename);
    int compileErrorCount() const;

    int syntaxCheckErrorCount() const;

    int compileIssueCount() const;

    int syntaxCheckIssueCount() const;

private slots:
    void onRunnerTerminated();
    void onCompileFinished();
    void onCompileIssue(PCompileIssue issue);
    void onSyntaxCheckFinished();
    void onSyntaxCheckIssue(PCompileIssue issue);

private:
    Compiler* mCompiler;
    int mCompileErrorCount;
    int mCompileIssueCount;
    int mSyntaxCheckErrorCount;
    int mSyntaxCheckIssueCount;
    Compiler* mBackgroundSyntaxChecker;
    Runner* mRunner;
    QRecursiveMutex mCompileMutex;
    QRecursiveMutex mBackgroundSyntaxCheckMutex;
    QRecursiveMutex mRunnerMutex;
};

class CompileError : public BaseError {
public:
    explicit CompileError(const QString& reason);
};

#endif // COMPILERMANAGER_H
