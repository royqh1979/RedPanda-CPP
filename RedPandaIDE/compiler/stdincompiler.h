#ifndef STDINCOMPILER_H
#define STDINCOMPILER_H

#include "compiler.h"

class StdinCompiler : public Compiler
{
    Q_OBJECT

public:
    explicit StdinCompiler(const QString& filename, const QString& content, bool silent,bool onlyCheckSyntax);

    // Compiler interface
protected:
    Settings::PCompilerSet compilerSet() override;
    bool prepareForCompile() override;

private:
    QString mContent;

    // Compiler interface
protected:
    QString pipedText();
};

#endif // STDINCOMPILER_H
