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
#include "xmakecompiler.h"
#include "../project.h"
#include "compilermanager.h"
#include "../systemconsts.h"
#include "qt_utils/charsetinfo.h"
#include "../editor.h"

#include <QDir>

XMakeCompiler::XMakeCompiler(std::shared_ptr<Project> project, bool silent, bool onlyCheckSyntax):
    Compiler("",silent,onlyCheckSyntax),
    mOnlyClean(false)
{
    setProject(project);
}

void XMakeCompiler::buildXMakeFile()
{
    //we are using custom make file, don't overwrite it
    if (mProject->options().useCustomMakefile && !mProject->options().customMakefile.isEmpty())
        return;

    QFile file(mProject->makeFileName());
    newXMakeFile(file);

}

void XMakeCompiler::newXMakeFile(QFile& file)
{
    // Create OBJ output directory
    if (!mProject->options().objectOutput.isEmpty()) {
        QDir(mProject->directory()).mkpath(mProject->options().objectOutput);
    }

    // Write more information to the log file than before
    log(tr("Building xmake.lua file..."));
    log("--------");
    log(tr("- Filename: %1").arg(mProject->xmakeFileName()));

    // Create the actual file
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        throw CompileError(tr("Can't open '%1' for write!").arg(mProject->xmakeFileName()));

    writeln(file,"-- Project: " + mProject->name());
    writeln(file,QString("-- xmake.lua created by Red Panda C++ ") + REDPANDA_CPP_VERSION);
    writeln(file);

    writeln(file, QString("target(\"%1\")").arg(mProject->name()));
    switch(mProject->options().type) {
    case ProjectType::StaticLib:
        writeln(file,"\tset_kind(\"static\")");
        break;
    case ProjectType::DynamicLib:
        writeln(file,"\tset_kind(\"shared\")");
        break;
    default:
        writeln(file,"\tset_kind(\"binary\")");
    }
    // add files to compile
    foreach(const PProjectUnit &unit, mProject->unitList()) {
        if (!unit->compile() && !unit->link())
            continue;

        // Only process source files
        QString relativeName = extractRelativePath(mProject->directory(), unit->fileName());
        FileType fileType = getFileType(relativeName);
        if (fileType==FileType::ASM && !compilerSet()->canAssemble())
            continue;

        if (fileType == FileType::CHeader || fileType == FileType::CppHeader) {
            writeln(file,QString("\tadd_headerfiles(\"%1\")").arg(relativeName));
        } else if (fileType == FileType::CSource || fileType == FileType::CppSource
                || fileType == FileType::ASM || fileType==FileType::GAS || fileType==FileType::WindowsResourceSource) {
            writeln(file,QString("\tadd_files(\"%1\")").arg(relativeName));
        }
    }

    // Get windres file
#ifdef Q_OS_WIN
    if (!mProject->options().privateResource.isEmpty()) {
        QString relativeName = extractRelativePath(mProject->directory(), mProject->options().privateResource);
        writeln(file,QString("\tadd_files(\"%1\")").arg(relativeName));
    }
#endif

    // Get list of applicable flags
    QString cCompileArguments = getCCompileArguments(mOnlyCheckSyntax);
    QString cppCompileArguments = getCppCompileArguments(mOnlyCheckSyntax);
    QString libraryArguments = getLibraryArguments(FileType::Project);
    QString cIncludeArguments = getCIncludeArguments() + " " + getProjectIncludeArguments();
    QString cppIncludeArguments = getCppIncludeArguments() + " " +getProjectIncludeArguments();

    cCompileArguments.replace("\"","\\\"");
    cppCompileArguments.replace("\"","\\\"");
    libraryArguments.replace("\"","\\\"");
    cIncludeArguments.replace("\"","\\\"");
    cppIncludeArguments.replace("\"","\\\"");

    writeln(file,QString("\tadd_cflags(\"%1\")").arg(cCompileArguments));
    writeln(file,QString("\tadd_cflags(\"%1\")").arg(cIncludeArguments));
    writeln(file,QString("\tadd_cxxflags(\"%1\")").arg(cppCompileArguments));
    writeln(file,QString("\tadd_cxxflags(\"%1\")").arg(cppIncludeArguments));
    writeln(file,QString("\tadd_ldflags(\"%1\")").arg(libraryArguments));

#ifdef Q_OS_WIN
    writeln(file,QString("\tadd_rcflags(\"%1\")").arg(mProject->options().resourceCmd));
#endif

    if (mProject->getCompileOption(CC_CMD_OPT_DEBUG_INFO) == COMPILER_OPTION_ON) {
        writeln(file,QString("\tadd_defines(\"__DEBUG__\")"));
    }

    writeln(file,QString("target_end()"));
}

void XMakeCompiler::writeln(QFile &file, const QString &s)
{
    if (!s.isEmpty())
        file.write(s.toLocal8Bit());
    file.write("\n");
}

bool XMakeCompiler::onlyClean() const
{
    return mOnlyClean;
}

void XMakeCompiler::setOnlyClean(bool newOnlyClean)
{
    mOnlyClean = newOnlyClean;
}

bool XMakeCompiler::prepareForRebuild()
{
    //we use make argument to clean
    return true;
}

bool XMakeCompiler::prepareForCompile()
{
    if (!mProject)
        return false;
    //initProgressForm();
    log(tr("Compiling project changes..."));
    log("--------");
    log(tr("- Project Filename: %1").arg(mProject->filename()));
    log(tr("- Compiler Set Name: %1").arg(compilerSet()->name()));
    log("");

    buildXMakeFile();

    mCompiler = compilerSet()->make();

    if (!fileExists(mCompiler)) {
        throw CompileError(
                    tr("Make program '%1' doesn't exists!").arg(mCompiler)
                    +"<br />"
                    +tr("Please check the \"program\" page of compiler settings."));
    }

    QString parallelParam;
    if (mProject->options().allowParallelBuilding) {
        if (mProject->options().parellelBuildingJobs==0) {
            parallelParam = " --jobs";
        } else {
            parallelParam = QString(" -j%1").arg(mProject->options().parellelBuildingJobs);
        }
    }

    if (mOnlyClean) {
        mArguments = QString(" %1 -f \"%2\" clean").arg(parallelParam,
                                                        extractRelativePath(
                                                            mProject->directory(),
                                                            mProject->makeFileName()));
    } else if (mRebuild) {
        mArguments = QString(" %1 -f \"%2\" clean all").arg(parallelParam,
                                                            extractRelativePath(
                                                            mProject->directory(),
                                                            mProject->makeFileName()));
    } else {
        mArguments = QString(" %1 -f \"%2\" all").arg(parallelParam,
                                                      extractRelativePath(
                                                      mProject->directory(),
                                                      mProject->makeFileName()));
    }
    mDirectory = mProject->directory();

    log(tr("Processing makefile:"));
    log("--------");
    log(tr("- makefile processer: %1").arg(mCompiler));
    log(tr("- Command: %1 %2").arg(extractFileName(mCompiler)).arg(mArguments));
    log("");

    return true;
}
