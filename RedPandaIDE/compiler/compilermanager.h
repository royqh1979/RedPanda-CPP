#ifndef COMPILERMANAGER_H
#define COMPILERMANAGER_H

#include <QObject>
#include <QMutex>

class ExecutableRunner;
class Compiler;
class CompilerManager : public QObject
{
    Q_OBJECT
public:
    explicit CompilerManager(QObject *parent = nullptr);

    bool compiling();
    bool backgroundSyntaxChecking();

    void compile(const QString& filename, const QByteArray& encoding, bool silent=false,bool onlyCheckSyntax=false);
    void run(const QString& filename, const QString& arguments, const QString& workDir);
    bool canCompile(const QString& filename);
private slots:
    void onCompileFinished();
    void onRunnerTerminated();
private:
    Compiler* mCompiler;
    Compiler* mBackgroundSyntaxChecker;
    ExecutableRunner* mRunner;
    QMutex compileMutex;
    QMutex backgroundSyntaxChekMutex;
    QMutex runnerMutex;
};

#endif // COMPILERMANAGER_H
