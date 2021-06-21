#include "filecompiler.h"
#include "utils.h"
#include "../mainwindow.h"
#include "compilermanager.h"

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>


FileCompiler::FileCompiler(const QString &filename, const QByteArray &encoding,bool silent,bool onlyCheckSyntax):
    Compiler(silent,onlyCheckSyntax),
    mFileName(filename),
    mEncoding(encoding)
{

}

Settings::PCompilerSet FileCompiler::compilerSet()
{
    return pSettings->compilerSets().defaultSet();
}

bool FileCompiler::prepareForCompile()
{
    log(tr("Compiling single file..."));
    log("------------------");
    log(tr("- Filename: %1").arg(mFileName));
    log(tr("- Compiler Set Name: %1").arg(compilerSet()->name()));
    log("");
    FileType fileType = getFileType(mFileName);
    mArguments= QString(" \"%1\"").arg(mFileName);
    if (!mOnlyCheckSyntax) {
        mOutputFile = getCompiledExecutableName(mFileName);
        mArguments+=QString(" -o \"%1\"").arg(mOutputFile);

        //remove the old file it exists
        QFile outputFile(mOutputFile);
        if (outputFile.exists()) {
            if (!outputFile.remove()) {
                error(tr("Can't delete the old executable file \"%1\".\n").arg(mOutputFile));
                return false;
            }
        }
    }

    mArguments += getCharsetArgument(mEncoding);
    QString strFileType;
    switch(fileType) {
    case FileType::CSource:
        mArguments += getCCompileArguments(mOnlyCheckSyntax);
        mArguments += getCIncludeArguments();
        strFileType = "C";
        mCompiler = compilerSet()->CCompiler();
        break;
    case FileType::CppSource:
        mArguments += getCCompileArguments(mOnlyCheckSyntax);
        mArguments += getCIncludeArguments();
        strFileType = "C++";
        mCompiler = compilerSet()->cppCompiler();
        break;
    default:
        throw CompileError(tr("Can't find the compiler for file %1").arg(mFileName));
    }
    mArguments += getLibraryArguments();

    if (!QFile(mCompiler).exists()) {
        throw CompileError(tr("The Compiler '%1' doesn't exists!").arg(mCompiler));
    }

    log(tr("Processing %1 source file:").arg(strFileType));
    log("------------------");
    log(tr("%1 Compiler: %2").arg(strFileType).arg(mCompiler));
    log(tr("Command: %1 %2").arg(QFileInfo(mCompiler).fileName()).arg(mArguments));
    return true;
}
