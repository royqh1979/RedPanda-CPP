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
#include "compilermanager.h"
#include "qsynedit/syntaxer/asm.h"
#include "../systemconsts.h"

#include <QFile>
#include <QFileInfo>
#include <QMessageBox>


FileCompiler::FileCompiler(const QString &filename, const QByteArray &encoding,
                           FileType fileType,
                           CppCompileType compileType, bool onlyCheckSyntax):
    Compiler{filename, onlyCheckSyntax},
    mEncoding{encoding},
    mCompileType{compileType},
    mFileType{fileType}
{

}

bool FileCompiler::prepareForCompile()
{
    QString oldDebugOptionValue = compilerSet()->getCompileOptionValue(CC_CMD_OPT_DEBUG_INFO);
    auto action = finally([this, oldDebugOptionValue]{
       compilerSet()->setCompileOption(CC_CMD_OPT_DEBUG_INFO,oldDebugOptionValue);
    });
    Settings::CompilerSet::CompilationStage stage = Settings::CompilerSet::CompilationStage::GenerateExecutable;
    switch(mCompileType) {
    case CppCompileType::PreprocessOnly:
        stage = Settings::CompilerSet::CompilationStage::PreprocessingOnly;
        break;
    case CppCompileType::GenerateAssemblyOnly:
        stage = Settings::CompilerSet::CompilationStage::CompilationProperOnly;
        if (pSettings->languages().noDebugDirectivesWhenGenerateASM())
            compilerSet()->setCompileOption(CC_CMD_OPT_DEBUG_INFO,COMPILER_OPTION_OFF);
        break;
    case CppCompileType::GenerateGimpleOnly:
        stage = Settings::CompilerSet::CompilationStage::GenerateGimple;
        break;
    default:
        break;
    }
    if (mOnlyCheckSyntax) {
        log(tr("Checking single file..."));
    } else {
        log(tr("Compiling single file..."));
    }
    log("------------------");
    log(tr("- Filename: %1").arg(mFilename));
    log(tr("- Compiler Set Name: %1").arg(compilerSet()->name()));
    log("");

    CompilerType compilerType = compilerSet()->compilerType();

    // GCC `import std;` sources should be added before the main file to generate GCM cache
    if (mFileType == FileType::CppSource &&
        (compilerType == CompilerType::GCC)) {
        mArguments += getCppGccImportStdSources(mOnlyCheckSyntax);
    }

    mArguments += QStringList{localizePath(mFilename)};
    if (!mOnlyCheckSyntax) {
        switch(stage) {
        case Settings::CompilerSet::CompilationStage::PreprocessingOnly:
            mOutputFile=changeFileExt(mFilename,compilerSet()->preprocessingSuffix());
            mArguments << "-E";
            break;
        case Settings::CompilerSet::CompilationStage::CompilationProperOnly:
            mOutputFile=changeFileExt(mFilename,compilerSet()->compilationProperSuffix());
            mArguments += {"-S", "-fverbose-asm"};
            break;
        case Settings::CompilerSet::CompilationStage::GenerateGimple:
            mOutputFile=changeFileExt(mFilename,compilerSet()->compilationProperSuffix());
            mArguments += {"-S", QString("-fdump-tree-gimple=%1").arg(localizePath(changeFileExt(mFilename,"gimple")))};
            break;
        case Settings::CompilerSet::CompilationStage::AssemblingOnly:
            mOutputFile=changeFileExt(mFilename,compilerSet()->assemblingSuffix());
            mArguments << "-c";
            break;
        case Settings::CompilerSet::CompilationStage::GenerateExecutable:
            mOutputFile = changeFileExt(mFilename,compilerSet()->executableSuffix());
        }
#ifdef ENABLE_SDCC
        if (compilerSet()->compilerType()==CompilerType::SDCC) {
            if (compilerSet()->executableSuffix()==SDCC_IHX_SUFFIX) {

            }
        }
#endif
        mOutputFile = localizePath(mOutputFile);
        mArguments += {"-o",  mOutputFile};

#if defined(ARCH_X86_64) || defined(ARCH_X86)
        if (mCompileType == CppCompileType::GenerateAssemblyOnly) {
            if (pSettings->languages().noSEHDirectivesWhenGenerateASM())
                mArguments << "-fno-asynchronous-unwind-tables";
            if (pSettings->languages().x86DialectOfASMGenerated()==Settings::Languages::X86ASMDialect::Intel)
                mArguments << "-masm=intel";
        }
#endif
        //remove the old file if it exists
        QFile outputFile(mOutputFile);
        if (outputFile.exists()) {
            if (!outputFile.remove()) {
                error(tr("Can't delete the old executable file \"%1\".\n").arg(mOutputFile));
                return false;
            }
        }
    }

    mArguments += getCharsetArgument(mEncoding, mFileType, mOnlyCheckSyntax);
    QString strFileType;
    switch(mFileType) {
    case FileType::GAS:
        mArguments += getCCompileArguments(mOnlyCheckSyntax);
        mArguments += getCIncludeArguments();
        mArguments += getProjectIncludeArguments();
        if (pSettings->compile().GASLinkCStandardLib()) {
            mArguments.removeAll("-nostdlib");
        } else {
            mArguments += "-nostdlib";
        }
        strFileType = tr("GNU Assembler");
        mCompiler = compilerSet()->CCompiler();
        break;
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
        mArguments += getLibraryArguments(mFileType);

//    if (isASMSourceFile(fileType)) {
//        bool hasStart=false;
//        QStringList lines=readFileToLines(mFilename);
//        QSynedit::ASMSyntaxer syntaxer;
//        syntaxer.resetState();
//        QString lastToken;
//        QString token;
//        QSynedit::PTokenAttribute attr;
//        for (int i=0;i<lines.count();i++) {
//            QString line=lines[i];
//            syntaxer.setLine(line,i+1);
//            lastToken="";
//            while(!syntaxer.eol()) {
//                token=syntaxer.getToken();
//                if (token==":" && lastToken=="_start") {
//                    hasStart=true;
//                    break;
//                }
//                attr = syntaxer.getTokenAttribute();
//                if (attr->tokenType() != QSynedit::TokenType::Space
//                        && attr->tokenType()!=QSynedit::TokenType::String
//                        && attr->tokenType()!=QSynedit::TokenType::Character)
//                    lastToken=token;
//                syntaxer.next();
//            }
//            if (hasStart)
//                break;
//        }
//        if (hasStart) {
//            mArguments << "-nostartfiles";
//        }
//    }

    if (!fileExists(mCompiler)) {
        throw CompileError(
                    tr("The Compiler '%1' doesn't exists!").arg(mCompiler)
                    +"<br />"
                    +tr("Please check the \"program\" page of compiler settings."));
    }

    log(tr("Processing %1 source file:").arg(strFileType));
    log("------------------");
    log(tr("%1 Compiler: %2").arg(strFileType,mCompiler));
    QString command = escapeCommandForLog(mCompiler, mArguments);
    log(tr("Command: %1").arg(command));
    mDirectory = extractFileDir(mFilename);
    return true;
}

bool FileCompiler::prepareForRebuild()
{
    QString exeName=compilerSet()->getOutputFilename(mFilename);

    QFile file(exeName);

    if (file.exists() && !file.remove()) {
        QFileInfo info(exeName);
        throw CompileError(tr("Can't delete the old executable file \"%1\".\n").arg(info.absoluteFilePath()));
    }
    return true;
}
