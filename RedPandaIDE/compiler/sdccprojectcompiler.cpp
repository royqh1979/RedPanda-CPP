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
        writeln(file,"$(BIN): $(OBJ)");
        writeln(file,"\t$(CC) $(LIBS) -o $(BIN) $(LINKOBJ)");
    } else {
        writeln(file,"$(IHX): $(OBJ)\n");
        writeln(file,"\t$(CC) $(LIBS) -o $(IHX) $(LINKOBJ)");
        if (suffix == SDCC_HEX_SUFFIX) {
            writeln(file,"$(BIN): $(IHX)");
            writeln(file,"\t$(PACKIHX) $(IHX) > $(BIN)");
        } else {
            writeln(file,"$(BIN): $(IHX)");
            writeln(file,"\t$(MAKEBIN) $(IHX) $(BIN)");
        }
    }
    writeMakeObjFilesRules(file);
}

void SDCCProjectCompiler::newMakeFile(QFile& file)
{
    // Create OBJ output directory
    if (!mProject->options().objectOutput.isEmpty()) {
        QDir(mProject->directory()).mkpath(mProject->options().objectOutput);
    }
    // Create executable output directory
    if (!mProject->options().exeOutput.isEmpty()) {
        QDir(mProject->directory()).mkpath(mProject->options().exeOutput);
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
    QString Objects;
    QString LinkObjects;
    QString cleanObjects;

    // Create a list of object files
    foreach(const PProjectUnit &unit, mProject->unitList()) {
        if (!unit->compile() && !unit->link())
            continue;

        // Only process source files
        QString RelativeName = extractRelativePath(mProject->directory(), unit->fileName());
        FileType fileType = getFileType(RelativeName);

        if (fileType == FileType::CSource || fileType == FileType::CppSource
                || fileType==FileType::GAS) {
            if (!mProject->options().objectOutput.isEmpty()) {
                // ofile = C:\MyProgram\obj\main.o
                QString fullObjFile = includeTrailingPathDelimiter(mProject->options().objectOutput)
                        + extractFileName(unit->fileName());
                QString relativeObjFile = extractRelativePath(mProject->directory(), changeFileExt(fullObjFile, SDCC_REL_SUFFIX));
                QString objFile = genMakePath2(relativeObjFile);
                Objects += ' ' + objFile;
#ifdef Q_OS_WIN
                cleanObjects += ' ' + genMakePath1(relativeObjFile).replace("/",QDir::separator());
#else
                cleanObjects += ' ' + genMakePath1(relativeObjFile);
#endif
                if (unit->link()) {
                    LinkObjects += ' ' + genMakePath1(relativeObjFile);
                }
            } else {
                Objects += ' ' + genMakePath2(changeFileExt(RelativeName, SDCC_REL_SUFFIX));
#ifdef Q_OS_WIN
                cleanObjects += ' ' + genMakePath1(changeFileExt(RelativeName, SDCC_REL_SUFFIX)).replace("/",QDir::separator());
#else
                cleanObjects += ' ' + genMakePath1(changeFileExt(RelativeName, SDCC_REL_SUFFIX));
#endif
                if (unit->link())
                    LinkObjects = LinkObjects + ' ' + genMakePath1(changeFileExt(RelativeName, SDCC_REL_SUFFIX));
            }
        }
    }

    Objects = Objects.trimmed();
    LinkObjects = LinkObjects.trimmed();


    // Get list of applicable flags
    QString cCompileArguments = getCCompileArguments(mOnlyCheckSyntax);
    QString libraryArguments = getLibraryArguments(FileType::Project);
    QString cIncludeArguments = getCIncludeArguments() + " " + getProjectIncludeArguments();

    if (cCompileArguments.indexOf(" -g3")>=0
            || cCompileArguments.startsWith("-g3")) {
        cCompileArguments += " -D__DEBUG__";
    }

    writeln(file,"CC       = " + extractFileName(compilerSet()->CCompiler()));
    writeln(file,QString("PACKIHX  = ") + PACKIHX_PROGRAM);
    writeln(file,QString("MAKEBIN  = ") + MAKEBIN_PROGRAM);

    writeln(file,"OBJ      = " + Objects);
    writeln(file,"LINKOBJ  = " + LinkObjects);
#ifdef Q_OS_WIN
    writeln(file,"CLEANOBJ  = " + cleanObjects +
            + " " + genMakePath1(extractRelativePath(mProject->makeFileName(), changeFileExt(mProject->executable(),SDCC_IHX_SUFFIX))).replace("/",QDir::separator())
            + " " + genMakePath1(extractRelativePath(mProject->makeFileName(), mProject->executable())).replace("/",QDir::separator()) );
#else
    writeln(file,"CLEANOBJ  = " + cleanObjects +
              + " " + genMakePath1(extractRelativePath(mProject->makeFileName(), mProject->executable())));
#endif
    libraryArguments.replace('\\', '/');
    writeln(file,"LIBS     = " + libraryArguments);
    cIncludeArguments.replace('\\', '/');
    writeln(file,"INCS     = " + cIncludeArguments);
    writeln(file,"IHX      = " + genMakePath1(extractRelativePath(mProject->makeFileName(), changeFileExt(mProject->executable(), SDCC_IHX_SUFFIX))));
    writeln(file,"BIN      = " + genMakePath1(extractRelativePath(mProject->makeFileName(), mProject->executable())));
    //writeln(file,"ENCODINGS = -finput-charset=utf-8 -fexec-charset='+GetSystemCharsetName);
    cCompileArguments.replace('\\', '/');
    writeln(file,"CFLAGS   = $(INCS) " + cCompileArguments);
    writeln(file, QString("RM       = ") + CLEAN_PROGRAM );

    writeln(file);
}

void SDCCProjectCompiler::writeMakeTarget(QFile &file)
{
    writeln(file, ".PHONY: all all-before all-after clean clean-custom");
    writeln(file);
    writeln(file, "all: all-before $(BIN) all-after");
    writeln(file);

}

void SDCCProjectCompiler::writeMakeIncludes(QFile &file)
{
    foreach(const QString& s, mProject->options().makeIncludes) {
        writeln(file, "include " + genMakePath1(s));
    }
    if (!mProject->options().makeIncludes.isEmpty()) {
        writeln(file);
    }
}

void SDCCProjectCompiler::writeMakeClean(QFile &file)
{
    writeln(file, "clean: clean-custom");
    QString target="$(CLEANOBJ)";

    writeln(file, QString("\t-$(RM) %1 > %2 2>&1").arg(target,NULL_FILE));
    writeln(file);
}

void SDCCProjectCompiler::writeMakeObjFilesRules(QFile &file)
{
    PCppParser parser = mProject->cppParser();
    QString precompileStr;

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
        QString objStr=genMakePath2(shortFileName);
        // if we have scanned it, use scanned info
        if (parser && parser->scannedFiles().contains(unit->fileName())) {
            QSet<QString> fileIncludes = parser->getFileIncludes(unit->fileName());
            foreach(const PProjectUnit &unit2, projectUnits) {
                if (unit2==unit)
                    continue;
                if (fileIncludes.contains(unit2->fileName())) {
                    objStr = objStr + ' ' + genMakePath2(extractRelativePath(mProject->makeFileName(),unit2->fileName()));
                }
            }
        } else {
            foreach(const PProjectUnit &unit2, projectUnits) {
                FileType fileType = getFileType(unit2->fileName());
                if (fileType == FileType::CHeader || fileType==FileType::CppHeader)
                    objStr = objStr + ' ' + genMakePath2(extractRelativePath(mProject->makeFileName(),unit2->fileName()));
            }
        }
        QString objFileName;
        QString objFileName2;
        if (!mProject->options().objectOutput.isEmpty()) {
            QString fullObjname = includeTrailingPathDelimiter(mProject->options().objectOutput) +
                    extractFileName(unit->fileName());
            objFileName = genMakePath2(extractRelativePath(mProject->makeFileName(), changeFileExt(fullObjname, SDCC_REL_SUFFIX)));
            objFileName2 = genMakePath1(extractRelativePath(mProject->makeFileName(), changeFileExt(fullObjname, SDCC_REL_SUFFIX)));
//            if (!extractFileDir(ObjFileName).isEmpty()) {
//                objStr = genMakePath2(includeTrailingPathDelimiter(extractFileDir(ObjFileName))) + objStr;
//            }
        } else {
            objFileName = genMakePath2(changeFileExt(shortFileName, SDCC_REL_SUFFIX));
            objFileName2 = genMakePath1(changeFileExt(shortFileName, SDCC_REL_SUFFIX));
        }

        objStr = objFileName + ": "+objStr+precompileStr;

        writeln(file,objStr);

        // Write custom build command
        if (unit->overrideBuildCmd() && !unit->buildCmd().isEmpty()) {
            QString BuildCmd = unit->buildCmd();
            BuildCmd.replace("<CRTAB>", "\n\t");
            writeln(file, '\t' + BuildCmd);
            // Or roll our own
        } else {
            if (fileType==FileType::CSource) {
                writeln(file, "\t$(CC) $(CFLAGS) -c " + genMakePath1(shortFileName));
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
            parallelParam = " --jobs";
        } else {
            parallelParam = QString(" -j%1").arg(mProject->options().parellelBuildingJobs);
        }
    }

    if (onlyClean()) {
        mArguments = QString(" %1 -f \"%2\" clean").arg(parallelParam,
                                                        extractRelativePath(
                                                            mProject->directory(),
                                                            mProject->makeFileName()));
    } else if (mRebuild) {
        mArguments = QString("  -f \"%1\" clean").arg(extractRelativePath(
                                                            mProject->directory(),
                                                            mProject->makeFileName()));
        mExtraCompilersList.append(mCompiler);
        mExtraOutputFilesList.append("");
        mExtraArgumentsList.append(QString(" %1 -f \"%2\" all").arg(parallelParam,
                                                            extractRelativePath(
                                                            mProject->directory(),
                                                            mProject->makeFileName())));
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
