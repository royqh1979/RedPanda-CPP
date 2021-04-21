#ifndef COMPILER_H
#define COMPILER_H

#include <QThread>
#include "settings.h"

class Compiler : public QThread
{
    Q_OBJECT
public:
    enum class TargetType {
        Invalid,
        cttNone,
        File,
        Project,
        StdIn
    };
    Compiler(bool silent,bool onlyCheckSyntax);

signals:
    void compileStarted();
    void compileFinished();
    void compileOutput(const QString& msg);
    void compileError(const QString& errorMsg);
public slots:
    void stopCompile();

protected:
    void run() override;

protected:
    virtual Settings::PCompilerSet compilerSet() = 0;
    virtual bool prepareForCompile() = 0;
    virtual QString getCharsetArgument(const QByteArray& encoding);
    virtual QString getCCompileArguments(bool checkSyntax);
    virtual QString getCppCompileArguments(bool checkSyntax);
    virtual QString getCIncludeArguments();
    virtual QString getCppIncludeArguments();
    virtual QString getLibraryArguments();
    void log(const QString& msg);
    void error(const QString& msg);
    void runCommand(const QString& cmd, const QString& arguments, const QString& workingDir, const QString& inputText=QString());

protected:
    bool mSilent;
    bool mOnlyCheckSyntax;
    QString mCompiler;
    QString mArguments;
    QString mOutputFile;

private:
    bool mStop;

};

#endif // COMPILER_H
