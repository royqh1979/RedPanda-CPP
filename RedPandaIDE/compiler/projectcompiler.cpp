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
#include "projectcompiler.h"
#include "../project.h"
#include "compilermanager.h"
#include "../systemconsts.h"
#include "qt_utils/charsetinfo.h"
#include "../editor.h"
#include "qt_utils/utils.h"
#include "utils.h"
#include "utils/escape.h"
#include "utils/parsearg.h"

#include <QDir>

ProjectCompiler::ProjectCompiler(std::shared_ptr<Project> project):
    Compiler("",false),
    mOnlyClean(false)
{
    setProject(project);
}

void ProjectCompiler::buildMakeFile()
{
    //we are using custom make file, don't overwrite it
    if (mProject->options().useCustomMakefile && !mProject->options().customMakefile.isEmpty())
        return;
    switch(mProject->options().type) {
    case ProjectType::StaticLib:
        createStaticMakeFile();
        break;
    case ProjectType::DynamicLib:
        createDynamicMakeFile();
        break;
    default:
        createStandardMakeFile();
    }
}

void ProjectCompiler::createStandardMakeFile()
{
    QFile file(mProject->makeFileName());
    bool genModuleDef;
    newMakeFile(file, genModuleDef);
    QString executable = extractRelativePath(mProject->makeFileName(), mProject->outputFilename());
    QString exeTarget = escapeFilenameForMakefileTarget(executable);
    QString exeCommand = escapeArgumentForMakefileRecipe(executable, false);
    writeln(file, exeTarget + ": $(OBJ)\n");
    if (!mOnlyCheckSyntax) {
        if (mProject->options().isCpp) {
            writeln(file, "\t$(CXX) $(LINKOBJ) -o " + exeCommand + " $(LIBS)");
        } else
            writeln(file, "\t$(CC) $(LINKOBJ) -o " + exeCommand + " $(LIBS)");
    }
    writeMakeObjFilesRules(file);
}

void ProjectCompiler::createStaticMakeFile()
{
    QFile file(mProject->makeFileName());
    newMakeFile(file);
    QString libFilename = extractRelativePath(mProject->makeFileName(), mProject->outputFilename());
    QString libTarget = escapeFilenameForMakefileTarget(libFilename);
    QString libCommand = escapeArgumentForMakefileRecipe(libFilename, false);
    writeln(file, libTarget + ": $(OBJ)");
    writeln(file, "\tar r " + libCommand + " $(LINKOBJ)");
    writeln(file, "\tranlib " + libCommand);
    writeMakeObjFilesRules(file);
}

void ProjectCompiler::createDynamicMakeFile()
{
    QFile file(mProject->makeFileName());
    bool genModuleDef;
    newMakeFile(file, genModuleDef);
    QString dynamicLibFilename = extractRelativePath(mProject->makeFileName(), mProject->outputFilename());
    QString dynamicLibTarget = escapeFilenameForMakefileTarget(dynamicLibFilename);
    QString dynamicLibCommand = escapeArgumentForMakefileRecipe(dynamicLibFilename, false);
    writeln(file, dynamicLibTarget + ": $(DEF) $(OBJ)");
    if (genModuleDef) {
        if (mProject->options().isCpp) {
            writeln(file, "\t$(CXX) -mdll $(LINKOBJ) -o " + dynamicLibCommand + " $(LIBS) $(DEF) -Wl,--output-def,$(OUTPUT_DEF),--out-implib,$(STATIC)");
        } else {
            writeln(file, "\t$(CC) -mdll $(LINKOBJ) -o " + dynamicLibCommand + " $(LIBS) $(DEF) -Wl,--output-def,$(OUTPUT_DEF),--out-implib,$(STATIC)");
        }
    } else {
        if (mProject->options().isCpp) {
            writeln(file, "\t$(CXX) -mdll $(LINKOBJ) -o " + dynamicLibCommand + " $(LIBS) $(DEF) -Wl,--out-implib,$(STATIC)");
        } else {
            writeln(file, "\t$(CC) -mdll $(LINKOBJ) -o " + dynamicLibCommand + " $(LIBS) $(DEF) -Wl,--out-implib,$(STATIC)");
        }
    }
    writeMakeObjFilesRules(file);
}

void ProjectCompiler::newMakeFile(QFile &file)
{
    bool dummy;
    newMakeFile(file, dummy);
}

void ProjectCompiler::newMakeFile(QFile& file, bool &genModuleDef)
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
    writeMakeDefines(file, genModuleDef);

    // Write PHONY and all targets
    writeMakeTarget(file);

    // Write list of includes
    writeMakeIncludes(file);

    // Write clean command
    writeMakeClean(file);

    // PCH
    if (mProject->options().usePrecompiledHeader
            && fileExists(mProject->options().precompiledHeader)) {
        QString pchH = extractRelativePath(mProject->makeFileName(), mProject->options().precompiledHeader);
        QString pchHCommand = escapeArgumentForMakefileRecipe(pchH, false);
        QString pch = extractRelativePath(mProject->makeFileName(), mProject->options().precompiledHeader + "." GCH_EXT);
        QString pchTarget = escapeFilenameForMakefileTarget(pch);
        QString pchCommand = escapeArgumentForMakefileRecipe(pch, false);
        writeln(file, pchTarget + ": $(PCH_H)");
        writeln(file, "\t$(CXX) -c " + pchHCommand + " -o " + pchCommand + " $(CXXFLAGS)");
        writeln(file);
    }
}

void ProjectCompiler::writeMakeHeader(QFile &file)
{
    writeln(file,"# Project: " + mProject->name());
    writeln(file,QString("# Makefile created by Red Panda C++ ") + REDPANDA_CPP_VERSION);
    writeln(file);
}

void ProjectCompiler::writeMakeDefines(QFile &file, bool &genModuleDef)
{
    // Get list of object files
    QStringList objects;
    QStringList LinkObjects;
    QStringList cleanObjects;
    QStringList moduleDefines;

    genModuleDef = false;

    // Create a list of object files
    foreach(const PProjectUnit &unit, mProject->unitList()) {

        // Only process source files
        FileType fileType = getFileType(unit->fileName());

        if (fileType == FileType::CSource || fileType == FileType::CppSource
                || fileType==FileType::GAS) {
            QString relativeName = extractRelativePath(mProject->directory(), unit->fileName());
            if (!mProject->options().folderForObjFiles.isEmpty()) {
                // ofile = C:\MyProgram\obj\main.o
                QString fullObjFile = includeTrailingPathDelimiter(mProject->options().folderForObjFiles)
                        + extractFileName(unit->fileName());
                QString relativeObjFile = extractRelativePath(mProject->directory(), changeFileExt(fullObjFile, OBJ_EXT));
                objects << relativeObjFile;
                cleanObjects << localizePath(relativeObjFile);
                if (unit->link()) {
                    LinkObjects << relativeObjFile;
                }
            } else {
                objects << changeFileExt(relativeName, OBJ_EXT);
                cleanObjects << localizePath(changeFileExt(relativeName, OBJ_EXT));
                if (unit->link())
                    LinkObjects << changeFileExt(relativeName, OBJ_EXT);
            }
        }
        if (fileType == FileType::ModuleDef)
            moduleDefines.append(extractRelativePath(mProject->makeFileName(), unit->fileName()));
    }
    // Get windres file
    QString objResFile;
    QString cleanRes;
#ifdef Q_OS_WIN
    if (!mProject->options().privateResource.isEmpty()) {
        if (!mProject->options().folderForObjFiles.isEmpty()) {
            QString fullResFile = includeTrailingPathDelimiter(mProject->options().folderForObjFiles) +
                    changeFileExt(mProject->options().privateResource, RES_EXT);

            QString relativeResFile = extractRelativePath(mProject->directory(), fullResFile);
            objResFile = relativeResFile;
            cleanRes = localizePath(changeFileExt(relativeResFile, RES_EXT));
        } else {
            objResFile = changeFileExt(mProject->options().privateResource, RES_EXT);
            cleanRes = localizePath(changeFileExt(mProject->options().privateResource, RES_EXT));
        }
    }
#endif

    // Mention progress in the logs
    if (!objResFile.isEmpty()) {
        QString absolutePath = generateAbsolutePath(mProject->directory(),objResFile);
        QString escaped = escapeArgumentForPlatformShell(absolutePath, false);
        log(tr("- Resource File: %1").arg(escaped));
    }
    log("");

    QString cxx = extractFileName(compilerSet()->cppCompiler());
    QString cc = extractFileName(compilerSet()->CCompiler());
#ifdef Q_OS_WIN
    QString windres = extractFileName(compilerSet()->resourceCompiler());
#endif

    // Get list of applicable flags
    QStringList cCompileArguments = getCCompileArguments(false);
    QStringList cxxCompileArguments = getCppCompileArguments(false);

    QStringList libraryArguments = getLibraryArguments(FileType::Project);
    QStringList cIncludeArguments = getCIncludeArguments();
    QStringList cxxIncludeArguments = getCppIncludeArguments();
#ifdef Q_OS_WIN
    QStringList resourceArguments = parseArguments(mProject->options().resourceCmd, devCppMacroVariables(), true);
#endif

    QString executable = extractRelativePath(mProject->makeFileName(), mProject->outputFilename());
    QString cleanExe = localizePath(executable);
    QString pchHeader = extractRelativePath(mProject->makeFileName(), mProject->options().precompiledHeader);
    QString pch = extractRelativePath(mProject->makeFileName(), mProject->options().precompiledHeader + "." GCH_EXT);

    // programs
    writeln(file, "CXX      = " + escapeArgumentForMakefileVariableValue(cxx, true));
    writeln(file, "CC       = " + escapeArgumentForMakefileVariableValue(cc, true));
#ifdef Q_OS_WIN
    writeln(file, "WINDRES  = " + escapeArgumentForMakefileVariableValue(windres, true));
#endif
    writeln(file, "RM       = " CLEAN_PROGRAM);

    // compiler flags
    writeln(file, "LIBS     = " + escapeArgumentsForMakefileVariableValue(libraryArguments));
    writeln(file, "INCS     = " + escapeArgumentsForMakefileVariableValue(cIncludeArguments));
    writeln(file, "CXXINCS  = " + escapeArgumentsForMakefileVariableValue(cxxIncludeArguments));
    writeln(file, "CXXFLAGS = $(CXXINCS) " + escapeArgumentsForMakefileVariableValue(cxxCompileArguments));
    writeln(file, "CFLAGS   = $(INCS) " + escapeArgumentsForMakefileVariableValue(cCompileArguments));
#ifdef Q_OS_WIN
    writeln(file, "WINDRESFLAGS = " + escapeArgumentsForMakefileVariableValue(resourceArguments));
#endif

    // objects referenced in prerequisites
    // do not use them in targets or command arguments, they have different escaping rules
    if (!objResFile.isEmpty()) {
        writeln(file, "RES      = " + escapeFilenameForMakefilePrerequisite(objResFile));
        writeln(file, "OBJ      = " + escapeFilenamesForMakefilePrerequisite(objects) + " $(RES)");
    } else {
        writeln(file, "OBJ      = " + escapeFilenamesForMakefilePrerequisite(objects));
    };
    writeln(file, "BIN      = " + escapeFilenameForMakefilePrerequisite(executable));
    if (mProject->options().usePrecompiledHeader
            && fileExists(mProject->options().precompiledHeader)){
        writeln(file, "PCH_H    = " + escapeFilenameForMakefilePrerequisite(pchHeader));
        writeln(file, "PCH      = " + escapeFilenameForMakefilePrerequisite(pch));
    }

    // object referenced in command arguments
    // use them in targets or prerequisites, they have different escaping rules
    if (!objResFile.isEmpty()) {
        writeln(file, "LINKOBJ  = " + escapeArgumentsForMakefileVariableValue(LinkObjects) + " " + escapeArgumentForMakefileVariableValue(objResFile, false));
        writeln(file, "CLEANOBJ = " + escapeArgumentsForMakefileVariableValue(cleanObjects) + " " + escapeArgumentForMakefileVariableValue(cleanRes, false) + " " + escapeArgumentForMakefileVariableValue(cleanExe, false));
    } else {
        writeln(file, "LINKOBJ  = " + escapeArgumentsForMakefileVariableValue(LinkObjects));
        writeln(file, "CLEANOBJ = " + escapeArgumentsForMakefileVariableValue(cleanObjects) + " " + escapeArgumentForMakefileVariableValue(cleanExe, false));
    };

    // This needs to be put in before the clean command.
    if (mProject->options().type == ProjectType::DynamicLib) {
        QString outputFileDir = extractFilePath(mProject->outputFilename());
        QString outputFilename = extractFileName(mProject->outputFilename());
        QString libOutputFile;
        if (!outputFilename.startsWith("lib")) {
            libOutputFile = includeTrailingPathDelimiter(outputFileDir) + "lib" + outputFilename;
        } else {
            libOutputFile = includeTrailingPathDelimiter(outputFileDir) + outputFilename;
        }
        if (QFileInfo(libOutputFile).absoluteFilePath()
                == mProject->directory())
            libOutputFile = extractFileName(libOutputFile);
        else
            libOutputFile = extractRelativePath(mProject->makeFileName(), libOutputFile);

        QString defFile = localizePath(changeFileExt(libOutputFile, DEF_EXT));
        QString staticFile = localizePath(changeFileExt(libOutputFile, LIB_EXT));
        writeln(file,"DEF      = " + escapeFilenamesForMakefilePrerequisite(moduleDefines));
        writeln(file,"STATIC   = " + escapeFilenameForMakefilePrerequisite(staticFile));

        genModuleDef = !moduleDefines.contains(defFile);
        if (genModuleDef)
            writeln(file,"OUTPUT_DEF = " + escapeFilenameForMakefilePrerequisite(defFile));
    }
    writeln(file);
}

void ProjectCompiler::writeMakeTarget(QFile &file)
{
    writeln(file, ".PHONY: all all-before all-after clean clean-custom");
    writeln(file);
    writeln(file, "all: all-before $(BIN) all-after");
    writeln(file);

}

void ProjectCompiler::writeMakeIncludes(QFile &file)
{
    foreach(const QString& s, mProject->options().makeIncludes) {
        writeln(file, "include " + escapeFilenameForMakefileInclude(s));
    }
    if (!mProject->options().makeIncludes.isEmpty()) {
        writeln(file);
    }
}

void ProjectCompiler::writeMakeClean(QFile &file)
{
    writeln(file, "clean: clean-custom");
    QString target="$(CLEANOBJ)";
    if (mProject->options().usePrecompiledHeader
            && fileExists(mProject->options().precompiledHeader)) {
        QString pch = extractRelativePath(mProject->makeFileName(), mProject->options().precompiledHeader + "." GCH_EXT);
        target += ' ' + escapeArgumentForMakefileRecipe(pch, false);
    }

    if (mProject->options().type == ProjectType::DynamicLib) {
        target +=" $(STATIC)";
    }
    writeln(file, QString("\t-$(RM) %1 > %2 2>&1").arg(target,NULL_FILE));
    writeln(file);
}

void ProjectCompiler::writeMakeObjFilesRules(QFile &file)
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
        QString objStr = escapeFilenameForMakefilePrerequisite(shortFileName);
        // if we have scanned it, use scanned info
        if (parser && parser->fileScanned(unit->fileName())) {
            QSet<QString> fileIncludes = parser->getIncludedFiles(unit->fileName());
            foreach(const PProjectUnit &unit2, projectUnits) {
                if (unit2==unit)
                    continue;
                if (fileIncludes.contains(unit2->fileName())) {
                    if (mProject->options().usePrecompiledHeader &&
                           unit2->fileName() == mProject->options().precompiledHeader)
                        precompileStr = " $(PCH) ";
                    else {
                        QString prereq = extractRelativePath(mProject->makeFileName(), unit2->fileName());
                        objStr = objStr + ' ' + escapeFilenameForMakefilePrerequisite(prereq);
                    }
                }
            }
        } else {
            foreach(const PProjectUnit &unit2, projectUnits) {
                FileType fileType = getFileType(unit2->fileName());
                if (fileType == FileType::CHeader || fileType==FileType::CppHeader) {
                    QString prereq = extractRelativePath(mProject->makeFileName(), unit2->fileName());
                    objStr = objStr + ' ' + escapeFilenameForMakefilePrerequisite(prereq);
                }
            }
        }
        QString objFileNameTarget;
        QString objFileNameCommand;
        if (!mProject->options().folderForObjFiles.isEmpty()) {
            QString fullObjname = includeTrailingPathDelimiter(mProject->options().folderForObjFiles) +
                    extractFileName(unit->fileName());
            QString objectFile = extractRelativePath(mProject->makeFileName(), changeFileExt(fullObjname, OBJ_EXT));
            objFileNameTarget = escapeFilenameForMakefileTarget(objectFile);
            objFileNameCommand = escapeArgumentForMakefileRecipe(objectFile, false);
        } else {
            QString objectFile = changeFileExt(shortFileName, OBJ_EXT);
            objFileNameTarget = escapeFilenameForMakefileTarget(objectFile);
            objFileNameCommand = escapeArgumentForMakefileRecipe(objectFile, false);
        }

        objStr = objFileNameTarget + ": " + objStr + precompileStr;

        writeln(file,objStr);

        // Write custom build command
        if (unit->overrideBuildCmd() && !unit->buildCmd().isEmpty()) {
            QString BuildCmd = unit->buildCmd();
            BuildCmd.replace("<CRTAB>", "\n\t");
            writeln(file, '\t' + BuildCmd);
            // Or roll our own
        } else {
            QString encodingStr;
            if (compilerSet()->compilerType() != CompilerType::Clang && mProject->options().addCharset) {
                QByteArray defaultSystemEncoding=pCharsetInfoManager->getDefaultSystemEncoding();
                QByteArray encoding = mProject->options().execEncoding;
                QByteArray targetEncoding;
                QByteArray sourceEncoding;
                if ( encoding == ENCODING_SYSTEM_DEFAULT || encoding.isEmpty()) {
                    targetEncoding = defaultSystemEncoding;
                } else if (encoding == ENCODING_UTF8_BOM) {
                    targetEncoding = "UTF-8";
                } else if (encoding == ENCODING_UTF16_BOM) {
                    targetEncoding = "UTF-16";
                } else if (encoding == ENCODING_UTF32_BOM) {
                    targetEncoding = "UTF-32";
                } else {
                    targetEncoding = encoding;
                }

                if (unit->realEncoding().isEmpty()) {
                    if (unit->encoding() == ENCODING_AUTO_DETECT) {
                        Editor* editor = mProject->unitEditor(unit);
                        if (editor && editor->fileEncoding()!=ENCODING_ASCII
                                && editor->fileEncoding()!=targetEncoding) {
                            sourceEncoding = editor->fileEncoding();
                        } else {
                            sourceEncoding = targetEncoding;
                        }
                    } else if (unit->encoding()==ENCODING_PROJECT) {
                        sourceEncoding=mProject->options().encoding;
                    } else if (unit->encoding()==ENCODING_SYSTEM_DEFAULT) {
                        sourceEncoding = defaultSystemEncoding;
                    } else if (unit->encoding()!=ENCODING_ASCII && !unit->encoding().isEmpty()) {
                        sourceEncoding = unit->encoding();
                    } else {
                        sourceEncoding = targetEncoding;
                    }
                } else if (unit->realEncoding()==ENCODING_ASCII) {
                    sourceEncoding = targetEncoding;
                } else {
                    sourceEncoding = unit->realEncoding();
                }
                if (sourceEncoding==ENCODING_SYSTEM_DEFAULT)
                    sourceEncoding = defaultSystemEncoding;

                if (sourceEncoding!=targetEncoding) {
                    encodingStr = QString(" -finput-charset=%1 -fexec-charset=%2")
                            .arg(QString(sourceEncoding),
                                 QString(targetEncoding));
                }
            }

            if (fileType==FileType::CSource || fileType==FileType::CppSource) {
                if (unit->compileCpp())
                    writeln(file, "\t$(CXX) -c " + escapeArgumentForMakefileRecipe(shortFileName, false) + " -o " + objFileNameCommand + " $(CXXFLAGS) " + encodingStr);
                else
                    writeln(file, "\t$(CC) -c " + escapeArgumentForMakefileRecipe(shortFileName, false) + " -o " + objFileNameCommand + " $(CFLAGS) " + encodingStr);
            } else if (fileType==FileType::GAS) {
                writeln(file, "\t$(CC) -c " + escapeArgumentForMakefileRecipe(shortFileName, false) + " -o " + objFileNameCommand + " $(CFLAGS) " + encodingStr);
            }
        }
    }

#ifdef Q_OS_WIN
    if (!mProject->options().privateResource.isEmpty()) {
        // Concatenate all resource include directories
        QString ResIncludes(" ");
        for (int i=0;i<mProject->options().resourceIncludes.count();i++) {
            QString filename = mProject->options().resourceIncludes[i];
            if (!filename.isEmpty())
                ResIncludes += " --include-dir " + escapeArgumentForMakefileRecipe(filename, false);
        }

        QString resFiles;
        // Concatenate all resource filenames (not created when syntax checking)
        foreach(const PProjectUnit& unit, mProject->unitList()) {
            if (getFileType(unit->fileName())!=FileType::WindowsResourceSource)
                continue;
            if (fileExists(unit->fileName())) {
                QString ResFile = extractRelativePath(mProject->makeFileName(), unit->fileName());
                resFiles = resFiles + escapeFilenameForMakefilePrerequisite(ResFile) + ' ';
            }
        }

        // Determine resource output file
        QString fullName;
        if (!mProject->options().folderForObjFiles.isEmpty()) {
            fullName = includeTrailingPathDelimiter(mProject->options().folderForObjFiles) +
                  changeFileExt(mProject->options().privateResource, RES_EXT);
        } else {
            fullName = changeFileExt(mProject->options().privateResource, RES_EXT);
        }
        QString objFile = extractRelativePath(mProject->filename(), fullName);
        QString objFileNameCommand = escapeArgumentForMakefileRecipe(objFile, false);
        QString objFileNameTarget = escapeFilenameForMakefileTarget(objFile);
        QString privRes = extractRelativePath(mProject->filename(), mProject->options().privateResource);
        QString privResNameCommand = escapeArgumentForMakefileRecipe(privRes, false);
        QString privResNamePrereq = escapeFilenameForMakefilePrerequisite(privRes);

        // Build final cmd
        QString windresArgs;

        if (mProject->getCompileOption(CC_CMD_OPT_POINTER_SIZE)=="32")
              windresArgs = " -F pe-i386";

        writeln(file);
        writeln(file, objFileNameTarget + ": " + privResNamePrereq + ' ' + resFiles);
        writeln(file, "\t$(WINDRES) -i " + privResNameCommand + windresArgs + " --input-format=rc -o " + objFileNameCommand + " -O coff $(WINDRESFLAGS)"
            + ResIncludes);
        writeln(file);
    }
#endif
}

void ProjectCompiler::writeln(QFile &file, const QString &s)
{
    if (!s.isEmpty())
        file.write(s.toLocal8Bit());
    file.write("\n");
}

bool ProjectCompiler::onlyClean() const
{
    return mOnlyClean;
}

void ProjectCompiler::setOnlyClean(bool newOnlyClean)
{
    mOnlyClean = newOnlyClean;
}

bool ProjectCompiler::prepareForRebuild()
{
    //we use make argument to clean
    return true;
}

bool ProjectCompiler::prepareForCompile()
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

    QString makefile = extractRelativePath(mProject->directory(), mProject->makeFileName());
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
    if (mOnlyClean) {
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
