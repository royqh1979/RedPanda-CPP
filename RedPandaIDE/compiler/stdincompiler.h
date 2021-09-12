#ifndef STDINCOMPILER_H
#define STDINCOMPILER_H

#include "compiler.h"

class StdinCompiler : public Compiler
{
    Q_OBJECT

public:
    explicit StdinCompiler(const QString& filename, const QString& content, bool isAscii, bool silent,bool onlyCheckSyntax);

    // Compiler interface
protected:
    bool prepareForCompile() override;

private:
    QString mContent;
    bool mIsAscii;

    // Compiler interface
protected:
    QString pipedText() override;

    // Compiler interface
protected:
    bool prepareForRebuild() override;
};

#endif // STDINCOMPILER_H
