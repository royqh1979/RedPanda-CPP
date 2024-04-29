/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "stdincompiler.h"
#include "compilermanager.h"
#include <QFile>
#include <QFileInfo>

StdinCompiler::StdinCompiler(const QString &filename,const QByteArray& encoding, const QString& content, bool onlyCheckSyntax):
    Compiler(filename, onlyCheckSyntax),
    mContent(content),
    mEncoding(encoding)
{
}

bool StdinCompiler::prepareForCompile()
{
    if (mOnlyCheckSyntax)
        log(tr("Checking file syntax..."));
    else
        log(tr("Compiling..."));
    log("------------------");
    log(tr("- Filename: %1").arg(mFilename));
    log(tr("- Compiler Set Name: %1").arg(compilerSet()->name()));
    log("");
    FileType fileType = getFileType(mFilename);
    if (fileType == FileType::Other)
        fileType = FileType::CppSource;
    QString strFileType;
    if (mEncoding!=ENCODING_ASCII) {
        mArguments += getCharsetArgument(mEncoding,fileType, mOnlyCheckSyntax);
    }
    switch(fileType) {
    case FileType::CSource:
        mArguments += {"-x", "c", "-"};
        mArguments += getCCompileArguments(mOnlyCheckSyntax);
        mArguments += getCIncludeArguments();
        mArguments += getProjectIncludeArguments();
        strFileType = "C";
        mCompiler = compilerSet()->CCompiler();
        break;
    case FileType::GAS:
        mArguments += {"-x", "assembler", "-"};
        mArguments += getCCompileArguments(mOnlyCheckSyntax);
        mArguments += getCIncludeArguments();
        mArguments += getProjectIncludeArguments();
        strFileType = "GAS";
        mCompiler = compilerSet()->CCompiler();
        break;
    case FileType::CppSource:
    case FileType::CppHeader:
    case FileType::CHeader:
        mArguments += {"-x", "c++", "-"};
        mArguments += getCppCompileArguments(mOnlyCheckSyntax);
        mArguments += getCppIncludeArguments();
        mArguments += getProjectIncludeArguments();
        strFileType = "C++";
        mCompiler = compilerSet()->cppCompiler();
        break;
    default:
        throw CompileError(tr("Can't find the compiler for file %1").arg(mFilename));
    }
    if (!mOnlyCheckSyntax)
        mArguments += getLibraryArguments(fileType);

    if (!fileExists(mCompiler)) {
        if (!mOnlyCheckSyntax)
            throw CompileError(tr("The Compiler '%1' doesn't exists!").arg(mCompiler));
        else
            return false;
    }

    log(tr("Processing %1 source file:").arg(strFileType));
    log("------------------");
    log(tr("%1 Compiler: %2").arg(strFileType).arg(mCompiler));
    QString command = escapeCommandForLog(mCompiler, mArguments);
    log(tr("Command: %1").arg(command));
    mDirectory = extractFileDir(mFilename);
    return true;
}

QByteArray StdinCompiler::pipedText()
{
    if (mEncoding == ENCODING_ASCII)
        return mContent.toLatin1();

    TextEncoder encoder(mEncoding);
    if (encoder.isValid()) {
        return encoder.encodeUnchecked(mContent);
    } else {
        return mContent.toLocal8Bit();
    }
}

bool StdinCompiler::prepareForRebuild()
{
    return true;
}
