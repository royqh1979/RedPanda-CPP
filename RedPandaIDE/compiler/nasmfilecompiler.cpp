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
#include "nasmfilecompiler.h"
#include "utils.h"
#include "compilermanager.h"
#include "../systemconsts.h"

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>


NASMFileCompiler::NASMFileCompiler(const QString &filename):
    Compiler(filename, false)
{

}

bool NASMFileCompiler::prepareForCompile()
{
    //compilerSet()->setCompilationStage(Settings::CompilerSet::CompilationStage::GenerateExecutable);

    if (mOnlyCheckSyntax) {
        qFatal("NASM can't check syntax!");
    }
    log(tr("Compiling single file..."));
    log("------------------");
    log(tr("- Filename: %1").arg(mFilename));
    log(tr("- Compiler Set Name: %1").arg(compilerSet()->name()));
    log("");

    QString strFileType = "C";
    mCompiler = pSettings->compile().NASMPath();

    QStringList arguments = getCCompileArguments(false);
#ifdef Q_OS_WIN
    if (arguments.contains("-m32") || QString("i686").compare(compilerSet()->target(), Qt::CaseInsensitive)==0
            || QString("i386").compare(compilerSet()->target(), Qt::CaseInsensitive)==0) {
        mArguments += {"-f","elf32"};
    } else if (arguments.contains("-m64")) {
        mArguments += {"-f","elf64"};
    } else {
        mArguments += {"-f","elf64"};
    }
#endif
    if (arguments.contains("-g3")) {
        mArguments += "-g";
    }


    if (!fileExists(mCompiler)) {
        throw CompileError(
                    tr("The NASM '%1' doesn't exists!").arg(mCompiler)
                    +"<br />"
                    +tr("Please check NASM settings."));
    }

    mOutputFile=changeFileExt(mFilename, compilerSet()->executableSuffix());
    mObjFilename= changeFileExt(mFilename, DEFAULT_ASSEMBLING_SUFFIX );

    mArguments += {mFilename, "-o", mObjFilename};
    mExtraCompilersList << compilerSet()->CCompiler();
    QStringList args = getLibraryArguments(FileType::CSource);
    if (pSettings->compile().NASMLinkCStandardLib()) {
        args.removeAll("-nostdlib");
    } else {
        args += "-nostdlib";
    }
    args += {mObjFilename, "-o", mOutputFile};
    mExtraArgumentsList << args;
    mExtraOutputFilesList << QString();
    log(tr("Processing %1 source file:").arg(strFileType));
    log("------------------");
    log(tr("- %1 Compiler: %2").arg(strFileType, mCompiler));
    QString command = escapeCommandForLog(mCompiler, mArguments);
    log(tr("- Command: %1").arg(command));
    mDirectory = extractFileDir(mFilename);
    mStartCompileTime = QDateTime::currentDateTime();
    return true;
}

bool NASMFileCompiler::beforeRunExtraCommand(int idx)
{
    if (idx==0) {
        QFileInfo file(mObjFilename);
        return file.exists() && (file.lastModified()>mStartCompileTime);
    }
    return true;
}

bool NASMFileCompiler::prepareForRebuild()
{
    QString exeName=compilerSet()->getOutputFilename(mFilename);

    QFile file(exeName);

    if (file.exists() && !file.remove()) {
        QFileInfo info(exeName);
        throw CompileError(tr("Can't delete the old executable file \"%1\".\n").arg(info.absoluteFilePath()));
    }

    QString objFileName = changeFileExt(mFilename, DEFAULT_ASSEMBLING_SUFFIX);
    QFile objFile = QFile(objFileName);
    if (objFile.exists() && !objFile.remove()) {
        QFileInfo info(objFileName);
        throw CompileError(tr("Can't delete the old object file \"%1\".\n").arg(info.absoluteFilePath()));
    }
    return true;
}
