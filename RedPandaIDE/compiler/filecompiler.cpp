#include "filecompiler.h"
#include "utils.h"
#include "../mainwindow.h"

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
        QString outputFilename = getCompiledExecutableName(mFileName);
        mArguments+=QString(" -o \"%1\"").arg(outputFilename);

        //remove the old file it exists
        QFile outputFile(outputFilename);
        if (outputFile.exists()) {
            if (!outputFile.remove()) {
                error(tr("Can't delete the old executable file \"%1\".\n").arg(outputFilename));
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
        error(tr("Can't the compiler for file %1").arg(mFileName));
        return false;
    }
    mArguments += getLibraryArguments();


    log(tr("Processing %1 source file:").arg(strFileType));
    log("------------------");
    log(tr("%1 Compiler: %2").arg(strFileType).arg(mCompiler));
    log(tr("Command: %1 %2").arg(QFileInfo(mCompiler).fileName()).arg(mArguments));
    return true;
}
