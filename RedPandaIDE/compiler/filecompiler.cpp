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
#include "filecompiler.h"
#include "utils.h"
#include "../mainwindow.h"
#include "compilermanager.h"

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>


FileCompiler::FileCompiler(const QString &filename, const QByteArray &encoding,bool silent,bool onlyCheckSyntax):
    Compiler(filename, silent,onlyCheckSyntax),
    mEncoding(encoding)
{

}

bool FileCompiler::prepareForCompile()
{
    log(tr("Compiling single file..."));
    log("------------------");
    log(tr("- Filename: %1").arg(mFilename));
    log(tr("- Compiler Set Name: %1").arg(compilerSet()->name()));
    log("");
    FileType fileType = getFileType(mFilename);
    mArguments= QString(" \"%1\"").arg(mFilename);
    if (!mOnlyCheckSyntax) {
        mOutputFile = getCompiledExecutableName(mFilename);
        mArguments+=QString(" -o \"%1\"").arg(mOutputFile);

        //remove the old file if it exists
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
        mArguments += getProjectIncludeArguments();
        strFileType = "C";
        mCompiler = compilerSet()->CCompiler();
        break;
    case FileType::CppSource:
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
        throw CompileError(tr("The Compiler '%1' doesn't exists!").arg(mCompiler));
    }

    log(tr("Processing %1 source file:").arg(strFileType));
    log("------------------");
    log(tr("%1 Compiler: %2").arg(strFileType).arg(mCompiler));
    log(tr("Command: %1 %2").arg(extractFileName(mCompiler)).arg(mArguments));
    mDirectory = extractFileDir(mFilename);
    return true;
}

QString FileCompiler::pipedText()
{
    return QString();
}

bool FileCompiler::prepareForRebuild()
{
    QString exeName = getCompiledExecutableName(mFilename);
    QFile file(exeName);

    if (file.exists() && !file.remove()) {
        QFileInfo info(exeName);
        throw CompileError(tr("Can't delete the old executable file \"%1\".\n").arg(info.absoluteFilePath()));
    }
    return true;
}
