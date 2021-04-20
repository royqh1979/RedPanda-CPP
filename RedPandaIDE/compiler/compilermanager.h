#ifndef COMPILERMANAGER_H
#define COMPILERMANAGER_H

#include <QObject>
#include "compiler.h"
#include <QMutex>

class CompilerManager : public QObject
{
    Q_OBJECT
public:
    explicit CompilerManager(QObject *parent = nullptr);

    bool compiling();
    bool backgroundSyntaxChecking();

    void compile(const QString& filename, const QByteArray& encoding, bool silent=false,bool onlyCheckSyntax=false);
private slots:
    void onCompileFinished();
private:
    Compiler* mCompiler;
    Compiler* mBackgroundSyntaxChecker;
    QMutex compileMutex;
    QMutex backgroundSyntaxChekMutex;
};

#endif // COMPILERMANAGER_H
