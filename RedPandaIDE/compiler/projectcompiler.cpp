#include "projectcompiler.h"
#include "project.h"
#include "compilermanager.h"
#include "../systemconsts.h"

#include <QDir>

ProjectCompiler::ProjectCompiler(std::shared_ptr<Project> project, bool silent, bool onlyCheckSyntax):
    Compiler("",silent,onlyCheckSyntax)
{
    setProject(project);
}

void ProjectCompiler::buildMakeFile()
{
    //we are using custom make file, don't overwrite it
    if (!mProject->options().customMakefile.isEmpty())
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
    newMakeFile(file);
    file.write("$(BIN): $(OBJ)\n");
    if (!mOnlyCheckSyntax) {
        if (mProject->options().useGPP) {
            writeln(file,"$(BIN): $(OBJ)");
            writeln(file,"\t$(CPP) $(LINKOBJ) -o \"$(BIN)\" $(LIBS)");
        } else
            writeln(file,"\t$(CC) $(LINKOBJ) -o \"$(BIN)\" $(LIBS)");
    }
    writeMakeObjFilesRules(file);
}

void ProjectCompiler::newMakeFile(QFile& file)
{
    // Create OBJ output directory
    if (!mProject->options().objectOutput.isEmpty()) {
        QDir(mProject->directory()).mkpath(mProject->options().objectOutput);
    }

    // Write more information to the log file than before
    log(tr("Building makefile..."));
    log("--------");
    log(tr("- Filename: %1").arg(mProject->makeFileName()));

    // Create the actual file
    if (!file.open(QFile::WriteOnly | QFile::Truncate))
        throw CompileError(tr("Can't open '%1' for write!").arg(mProject->makeFileName());

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

void ProjectCompiler::writeMakeHeader(QFile &file)
{
    writeln(file,"# Project: " + mProject->name());
    writeln(file,QString("# Makefile created by Red Panda Dev-C++ ") + DEVCPP_VERSION);
    writeln(file);
    if (mOnlyCheckSyntax) {
        writeln(file,"# This Makefile is written for syntax check!");
        writeln(file,"# Regenerate it if you want to use this Makefile to build.");
        writeln(file);
    }
}

void ProjectCompiler::writeMakeDefines(QFile &file)
{
    // Get list of object files
    QString Objects;
    QString LinkObjects;

    // Create a list of object files
    for (int i=0;i<mProject->units().count();i++) {
        PProjectUnit unit = mProject[i];
        if (!unit->compile() && !unit->link())
            continue;

        // Only process source files
        QString RelativeName = extractRelativePath(mProject->directory(), unit->fileName());
        FileType fileType = getFileType(RelativeName);
        if (fileType == FileType::CSource || fileType == FileType::CppSource) {
            if (!mProject->options().objectOutput.isEmpty()) {
                // ofile = C:\MyProgram\obj\main.o
                QString ObjFile = includeTrailingPathDelimiter(mProject->options().objectOutput)
                        + extractFileName(unit->fileName());
                ObjFile = genMakePath1(extractRelativePath(mProject->directory(), changeFileExt(ObjFile, OBJ_EXT)));
                Objects += ' ' + ObjFile;

                if (unit->link())
                    LinkObjects += ' ' + ObjFile;
            } else {
                Objects += ' ' + genMakePath1(changeFileExt(RelativeName, OBJ_EXT));
                if (unit->link())
                    LinkObjects = LinkObjects + ' ' + genMakePath1(changeFileExt(RelativeName, OBJ_EXT));
            }
        }
    }

    Objects = Objects.trimmed();
    LinkObjects = LinkObjects.trimmed();

    // Get windres file
    QString ObjResFile;
    if (!mProject->options().privateResource.isEmpty()) {
      if (!mProject->options().objectOutput.isEmpty()) {
          ObjResFile = includeTrailingPathDelimiter(mProject->options().objectOutput) +
                  changeFileExt(mProject->options().privateResource, RES_EXT);
      } else
          ObjResFile = changeFileExt(mProject->options().privateResource, RES_EXT);
    }

    // Mention progress in the logs
    if (!ObjResFile.isEmpty()) {
        log(tr("- Resource File: %1").arg(QDir(mProject->directory()).absoluteFilePath(ObjResFile)));
    }
    log("");

    // Get list of applicable flags
    QString  cCompileArguments = getCCompileArguments(mOnlyCheckSyntax);
    QString cppCompileArguments = getCppCompileArguments(mOnlyCheckSyntax);
    QString libraryArguments = getLibraryArguments();
    QString cIncludeArguments = getCIncludeArguments();
    QString cppIncludeArguments = getCppIncludeArguments();
    QString projectIncludeArguments = getProjectIncludeArguments();

    if (Pos(' -g3', fCompileParams) > 0) or (Pos('-g3', fCompileParams) = 1) then begin
      Writeln(F, 'CPP      = ' + fCompilerSet.gppName + ' -D__DEBUG__');
      Writeln(F, 'CC       = ' + fCompilerSet.gccName + ' -D__DEBUG__');
    end else begin
      Writeln(F, 'CPP      = ' + fCompilerSet.gppName);
      Writeln(F, 'CC       = ' + fCompilerSet.gccName);
    end;
    Writeln(F, 'WINDRES  = ' + fCompilerSet.windresName);
    if (ObjResFile <> '') then begin
      Writeln(F, 'RES      = ' + GenMakePath1(ObjResFile));
      Writeln(F, 'OBJ      = ' + Objects + ' $(RES)');
      Writeln(F, 'LINKOBJ  = ' + LinkObjects + ' $(RES)');
    end else begin
      Writeln(F, 'OBJ      = ' + Objects);
      Writeln(F, 'LINKOBJ  = ' + LinkObjects);
    end;
    Writeln(F, 'LIBS     = ' + StringReplace(fLibrariesParams, '\', '/', [rfReplaceAll]));
    Writeln(F, 'INCS     = ' + StringReplace(fIncludesParams, '\', '/', [rfReplaceAll]));
    Writeln(F, 'CXXINCS  = ' + StringReplace(fCppIncludesParams, '\', '/', [rfReplaceAll]));
    Writeln(F, 'BIN      = ' + GenMakePath1(ExtractRelativePath(Makefile, fProject.Executable)));
    Writeln(F, 'CXXFLAGS = $(CXXINCS) ' + fCppCompileParams);
    Writeln(F, 'ENCODINGS = -finput-charset=utf-8 -fexec-charset='+GetSystemCharsetName);
    Writeln(F, 'CFLAGS   = $(INCS) ' + fCompileParams);
  //  Writeln(F, 'RM       = ' + CLEAN_PROGRAM + ' -f'); // TODO: use del or rm?
    Writeln(F, 'RM       = ' + CLEAN_PROGRAM + ' /f'); // TODO: use del or rm?
    if fProject.Options.UsePrecompiledHeader then begin
      Writeln(F, 'PCH_H = ' + fProject.Options.PrecompiledHeader );
      Writeln(F, 'PCH = ' + fProject.Options.PrecompiledHeader +'.gch' );
    end;

    // This needs to be put in before the clean command.
    if fProject.Options.typ = dptDyn then begin
      OutputFileDir := ExtractFilePath(Project.Executable);
      LibOutputFile := OutputFileDir + 'lib' + ExtractFileName(Project.Executable);
      if FileSamePath(LibOutputFile, Project.Directory) then
        LibOutputFile := ExtractFileName(LibOutputFile)
      else
        LibOutputFile := ExtractRelativePath(Makefile, LibOutputFile);

      Writeln(F, 'DEF      = ' + GenMakePath1(ChangeFileExt(LibOutputFile, DEF_EXT)));
      Writeln(F, 'STATIC   = ' + GenMakePath1(ChangeFileExt(LibOutputFile, LIB_EXT)));
    end;
    Writeln(F);
}

void ProjectCompiler::writeln(QFile &file, const QString &s)
{
    if (!s.isEmpty())
        file.write(s.toLocal8Bit());
    file.write("\n");
}

QString ProjectCompiler::pipedText()
{
    return QString();
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
    if (mRebuild) {
        mArguments = QString("-f \"%1\" clean all").arg(mProject->makeFileName());
    } else {
        mArguments = QString("-f \"%1\" all").arg(mProject->makeFileName());
    }
}
