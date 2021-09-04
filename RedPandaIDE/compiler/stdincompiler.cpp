#include "stdincompiler.h"
#include "compilermanager.h"
#include <QFile>
#include <QFileInfo>
#include "../platform.h"

StdinCompiler::StdinCompiler(const QString &filename, const QString& content,bool isAscii, bool silent, bool onlyCheckSyntax):
    Compiler(filename,silent,onlyCheckSyntax),
    mContent(content),
    mIsAscii(isAscii)
{

}

Settings::PCompilerSet StdinCompiler::compilerSet()
{
    return pSettings->compilerSets().defaultSet();
}

bool StdinCompiler::prepareForCompile()
{
    log(tr("Checking file syntax..."));
    log("------------------");
    log(tr("- Filename: %1").arg(mFilename));
    log(tr("- Compiler Set Name: %1").arg(compilerSet()->name()));
    log("");
    FileType fileType = getFileType(mFilename);
    if (fileType == FileType::Other)
        fileType = FileType::CppSource;
    QString strFileType;
    if (!mIsAscii)
        mArguments += getCharsetArgument(getDefaultSystemEncoding());
    switch(fileType) {
    case FileType::CSource:
    case FileType::CHeader:
        mArguments += " -x c - ";
        mArguments += getCCompileArguments(mOnlyCheckSyntax);
        mArguments += getCIncludeArguments();
        strFileType = "C";
        mCompiler = compilerSet()->CCompiler();
        break;
    case FileType::CppSource:
    case FileType::CppHeader:
        mArguments += " -x c++ - ";
        mArguments += getCCompileArguments(mOnlyCheckSyntax);
        mArguments += getCIncludeArguments();
        strFileType = "C++";
        mCompiler = compilerSet()->cppCompiler();
        break;
    default:
        throw CompileError(tr("Can't find the compiler for file %1").arg(mFilename));
    }
    mArguments += getLibraryArguments(fileType);

    if (!QFile(mCompiler).exists()) {
        throw CompileError(tr("The Compiler '%1' doesn't exists!").arg(mCompiler));
    }

    log(tr("Processing %1 source file:").arg(strFileType));
    log("------------------");
    log(tr("%1 Compiler: %2").arg(strFileType).arg(mCompiler));
    log(tr("Command: %1 %2").arg(QFileInfo(mCompiler).fileName()).arg(mArguments));
    return true;
}

QString StdinCompiler::pipedText()
{
    return mContent;
}

bool StdinCompiler::prepareForRebuild()
{
    return true;
}
