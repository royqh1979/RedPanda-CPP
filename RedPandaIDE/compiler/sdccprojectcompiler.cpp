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
#include "sdccprojectcompiler.h"
#include "../project.h"
#include "compilermanager.h"
#include "../systemconsts.h"
#include "qt_utils/utils.h"
#include "utils.h"
#include "utils/escape.h"

#include <QDir>

SDCCProjectCompiler::SDCCProjectCompiler(std::shared_ptr<Project> project):
    ProjectCompiler(project)
{
}

void SDCCProjectCompiler::buildMakeFile()
{
    //we are using custom make file, don't overwrite it
    if (mProject->options().useCustomMakefile && !mProject->options().customMakefile.isEmpty())
        return;
    createStandardMakeFile();
}

void SDCCProjectCompiler::createStandardMakeFile()
{
    QFile file(mProject->makeFileName());
    newMakeFile(file);
    QString suffix = compilerSet()->executableSuffix();
    if (suffix == SDCC_IHX_SUFFIX) {
        writeln(file,"$(BIN_TAR): $(OBJ)");
        writeln(file,"\t$(CC) $(LIBS) -o $(BIN_ARG) $(LINKOBJ)");
    } else {
        writeln(file,"$(IHX_TAR): $(OBJ)\n");
        writeln(file,"\t$(CC) $(LIBS) -o $(IHX_ARG) $(LINKOBJ)");
        if (suffix == SDCC_HEX_SUFFIX) {
            writeln(file,"$(BIN_TAR): $(IHX_DEP)");
            writeln(file,"\t$(PACKIHX) $(IHX_ARG) > $(BIN_ARG)");
        } else {
            writeln(file,"$(BIN_TAR): $(IHX_DEP)");
            writeln(file,"\t$(MAKEBIN) $(IHX_ARG) $(BIN_ARG)");
        }
    }
    writeMakeObjFilesRules(file);
}

void SDCCProjectCompiler::newMakeFile(QFile& file)
{
    // Create OBJ output directory
    if (!mProject->options().folderForObjFiles.isEmpty()) {
        QDir(mProject->directory()).mkpath(mProject->options().folderForObjFiles);
    }
    // Create executable output directory
    if (!mProject->options().folderForOutput.isEmpty()) {
        QDir(mProject->directory()).mkpath(mProject->options().folderForOutput);
    }
    // Write more information to the log file than before
    log(tr("Building makefile..."));
    log("--------");
    log(tr("- Filename: %1").arg(mProject->makeFileName()));

    // Create the actual file
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        throw CompileError(tr("Can't open '%1' for write!").arg(mProject->makeFileName()));

    // Write header
    writeMakeHeader(file);

    // Writes definition list
    writeMakeDefines(file);

    // Write PHONY and all targets
    writeMakeTarget(file);

    // Write list of includes
    writeMakeIncludes(file);

    // Write clean command
    writeMakeClean(file);

}

void SDCCProjectCompiler::writeMakeHeader(QFile &file)
{
    writeln(file,"# Project: " + mProject->name());
    writeln(file,QString("# Makefile created by Red Panda C++ ") + REDPANDA_CPP_VERSION);
    writeln(file);
}

void SDCCProjectCompiler::writeMakeDefines(QFile &file)
{
    // Get list of object files
    QStringList Objects;
    QStringList LinkObjects;
    QStringList cleanObjects;

    // Create a list of object files
    foreach(const PProjectUnit &unit, mProject->unitList()) {
        if (!unit->compile() && !unit->link())
            continue;

        // Only process source files
        QString RelativeName = extractRelativePath(mProject->directory(), unit->fileName());
        FileType fileType = getFileType(RelativeName);

        if (fileType == FileType::CSource || fileType == FileType::CppSource
                || fileType==FileType::GAS) {
            if (!mProject->options().folderForObjFiles.isEmpty()) {
                // ofile = C:\MyProgram\obj\main.o
                QString fullObjFile = includeTrailingPathDelimiter(mProject->options().folderForObjFiles)
                        + extractFileName(unit->fileName());
                QString relativeObjFile = extractRelativePath(mProject->directory(), changeFileExt(fullObjFile, SDCC_REL_SUFFIX));
                Objects << relativeObjFile;
                cleanObjects << localizePath(relativeObjFile);
                if (unit->link()) {
                    LinkObjects << relativeObjFile;
                }
            } else {
                Objects << changeFileExt(RelativeName, SDCC_REL_SUFFIX);
                cleanObjects << localizePath(changeFileExt(RelativeName, SDCC_REL_SUFFIX));
                if (unit->link())
                    LinkObjects << changeFileExt(RelativeName, SDCC_REL_SUFFIX);
            }
        }
    }

    QString cc = extractFileName(compilerSet()->CCompiler());

    QStringList cCompileArguments = getCCompileArguments(mOnlyCheckSyntax);
    QStringList libraryArguments = getLibraryArguments(FileType::Project);
    QStringList cIncludeArguments = getCIncludeArguments() + getProjectIncludeArguments();

    QString executable = extractRelativePath(mProject->makeFileName(), mProject->outputFilename());
    QString cleanExe = localizePath(executable);
    QString ihx = extractRelativePath(mProject->makeFileName(), changeFileExt(mProject->outputFilename(), SDCC_IHX_SUFFIX));
    QString cleanIhx = localizePath(ihx);

    writeln(file, "CC       = " + escapeArgumentForMakefileVariableValue(cc, true));
    writeln(file, "PACKIHX  = " PACKIHX_PROGRAM);
    writeln(file, "MAKEBIN  = " MAKEBIN_PROGRAM);

    writeln(file, "OBJ      = " + escapeFilenamesForMakefilePrerequisite(Objects));
    writeln(file, "LINKOBJ  = " + escapeArgumentsForMakefileVariableValue(LinkObjects));
    writeln(file,"CLEANOBJ = " + escapeArgumentsForMakefileVariableValue(cleanObjects) + ' ' +
        escapeArgumentForMakefileVariableValue(cleanIhx, false) + ' ' +
        escapeArgumentForMakefileVariableValue(cleanExe, false));
    writeln(file, "LIBS     = " + escapeArgumentsForMakefileVariableValue(libraryArguments));
    writeln(file, "INCS     = " + escapeArgumentsForMakefileVariableValue(cIncludeArguments));
    writeln(file, "IHX_TAR  = " + escapeFilenameForMakefileTarget(ihx));
    writeln(file, "IHX_DEP  = " + escapeFilenameForMakefilePrerequisite(ihx));
    writeln(file, "IHX_ARG  = " + escapeArgumentForMakefileVariableValue(ihx, false));
    writeln(file, "BIN_TAR  = " + escapeFilenameForMakefileTarget(executable));
    writeln(file, "BIN_DEP  = " + escapeFilenameForMakefilePrerequisite(executable));
    writeln(file, "BIN_ARG  = " + escapeArgumentForMakefileVariableValue(executable, false));
    writeln(file, "CFLAGS   = $(INCS) " + escapeArgumentsForMakefileVariableValue(cCompileArguments));
    writeln(file, "RM       = " CLEAN_PROGRAM);

    writeln(file);
}

void SDCCProjectCompiler::writeMakeTarget(QFile &file)
{
    writeln(file, ".PHONY: all all-before all-after clean clean-custom");
    writeln(file);
    writeln(file, "all: all-before $(BIN_DEP) all-after");
    writeln(file);

}

void SDCCProjectCompiler::writeMakeIncludes(QFile &file)
{
    foreach(const QString& s, mProject->options().makeIncludes) {
        writeln(file, "include " + escapeFilenameForMakefileInclude(s));
    }
    if (!mProject->options().makeIncludes.isEmpty()) {
        writeln(file);
    }
}

void SDCCProjectCompiler::writeMakeClean(QFile &file)
{
    writeln(file, "clean: clean-custom");
    writeln(file, QString("\t-$(RM) $(CLEANOBJ) > %1 2>&1").arg(NULL_FILE));
    writeln(file);
}

void SDCCProjectCompiler::writeMakeObjFilesRules(QFile &file)
{
    PCppParser parser = mProject->cppParser();

    QList<PProjectUnit> projectUnits=mProject->unitList();
    foreach(const PProjectUnit &unit, projectUnits) {
        if (!unit->compile())
            continue;
        FileType fileType = getFileType(unit->fileName());
        // Only process source files
        if (fileType!=FileType::CSource && fileType!=FileType::CppSource
                && fileType!=FileType::GAS)
            continue;

        QString shortFileName = extractRelativePath(mProject->makeFileName(),unit->fileName());

        writeln(file);
        QString objStr = escapeFilenameForMakefilePrerequisite(shortFileName);
        // if we have scanned it, use scanned info
        if (parser && parser->fileScanned(unit->fileName())) {
            QSet<QString> fileIncludes = parser->getIncludedFiles(unit->fileName());
            foreach(const PProjectUnit &unit2, projectUnits) {
                if (unit2==unit)
                    continue;
                if (fileIncludes.contains(unit2->fileName())) {
                    QString header = extractRelativePath(mProject->makeFileName(),unit2->fileName());
                    objStr = objStr + ' ' + escapeFilenameForMakefilePrerequisite(header);
                }
            }
        } else {
            foreach(const PProjectUnit &unit2, projectUnits) {
                FileType fileType = getFileType(unit2->fileName());
                if (fileType == FileType::CHeader || fileType==FileType::CppHeader) {
                    QString header = extractRelativePath(mProject->makeFileName(),unit2->fileName());
                    objStr = objStr + ' ' + escapeFilenameForMakefilePrerequisite(header);
                }
            }
        }
        QString objFileNameTarget;
        QString objFileNameCommand;
        if (!mProject->options().folderForObjFiles.isEmpty()) {
            QString fullObjname = includeTrailingPathDelimiter(mProject->options().folderForObjFiles) +
                    extractFileName(unit->fileName());
            QString objFile = extractRelativePath(mProject->makeFileName(), changeFileExt(fullObjname, SDCC_REL_SUFFIX));
            objFileNameTarget = escapeFilenameForMakefileTarget(objFile);
            objFileNameCommand = escapeArgumentForMakefileRecipe(objFile, false);
        } else {
            QString objFile = changeFileExt(shortFileName, SDCC_REL_SUFFIX);
            objFileNameTarget = escapeFilenameForMakefileTarget(objFile);
            objFileNameCommand = escapeArgumentForMakefileRecipe(objFile, false);
        }

        objStr = objFileNameTarget + ": " + objStr;

        writeln(file, objStr);

        // Write custom build command
        if (unit->overrideBuildCmd() && !unit->buildCmd().isEmpty()) {
            QString BuildCmd = unit->buildCmd();
            BuildCmd.replace("<CRTAB>", "\n\t");
            writeln(file, '\t' + BuildCmd);
            // Or roll our own
        } else {
            if (fileType==FileType::CSource) {
                writeln(file, "\t$(CC) $(CFLAGS) -c " + escapeArgumentForMakefileRecipe(shortFileName, false));
            }
        }
    }

}

void SDCCProjectCompiler::writeln(QFile &file, const QString &s)
{
    if (!s.isEmpty())
        file.write(s.toLocal8Bit());
    file.write("\n");
}

bool SDCCProjectCompiler::prepareForRebuild()
{
    //we use make argument to clean
    return true;
}

bool SDCCProjectCompiler::prepareForCompile()
{
    if (!mProject)
        return false;
    //initProgressForm();
    log(tr("Compiling project changes..."));
    log("--------");
    log(tr("- Project Filename: %1").arg(mProject->filename()));
    log(tr("- Compiler Set Name: %1").arg(compilerSet()->name()));
    log("");

    buildMakeFile();

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
            parallelParam = "--jobs";
        } else {
            parallelParam = QString("-j%1").arg(mProject->options().parellelBuildingJobs);
        }
    } else {
        parallelParam = "-j1";
    }

    QString makefile = 
            extractRelativePath(mProject->directory(), mProject->makeFileName());
    QStringList cleanArgs{
        "-f",
        makefile,
        "clean",
    };
    QStringList makeAllArgs{
        parallelParam,
        "-f",
        makefile,
        "all",
    };
    if (onlyClean()) {
        mArguments = cleanArgs;
    } else if (mRebuild) {
        mArguments = cleanArgs;
        mExtraCompilersList << mCompiler;
        mExtraOutputFilesList << "";
        mExtraArgumentsList << makeAllArgs;
    } else {
        mArguments = makeAllArgs;
    }
    mDirectory = mProject->directory();

    mOutputFile = mProject->outputFilename();
    log(tr("Processing makefile:"));
    log("--------");
    log(tr("- makefile processer: %1").arg(mCompiler));
    QString command = escapeCommandForLog(mCompiler, mArguments);
    log(tr("- Command: %1").arg(command));
    log("");

    return true;
}
