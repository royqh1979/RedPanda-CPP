#ifndef FILECOMPILER_H
#define FILECOMPILER_H

#include "compiler.h"

class FileCompiler : public Compiler
{
    Q_OBJECT
public:
    FileCompiler(const QString& filename, const QByteArray& encoding,bool silent,bool onlyCheckSyntax);

    // Compiler interface
protected:
    Settings::PCompilerSet compilerSet() override;
    bool prepareForCompile() override;

private:
    QByteArray mEncoding;

    // Compiler interface
protected:
    QString pipedText() override;

    // Compiler interface
protected:
    bool prepareForRebuild() override;
};

#endif // FILECOMPILER_H
