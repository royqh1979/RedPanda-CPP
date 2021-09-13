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
        PProjectUnit unit = mProject->units()[i];
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
    QString libraryArguments = getLibraryArguments(FileType::Project);
    QString cIncludeArguments = getCIncludeArguments() + " " + getProjectIncludeArguments();
    QString cppIncludeArguments = getCppIncludeArguments() + " " +getProjectIncludeArguments();

    if (cCompileArguments.indexOf(" -g3")>=0
            || cCompileArguments.startsWith("-g3")) {
        cCompileArguments += " -D__DEBUG__";
        cppCompileArguments+= " -D__DEBUG__";
    }
    writeln(file,"CPP      = " + compilerSet()->cppCompiler());
    writeln(file,"CC       = " + compilerSet()->CCompiler());
    writeln(file,"WINDRES  = " + compilerSet()->resourceCompiler());
    if (!ObjResFile.isEmpty()) {
      writeln(file,"RES      = " + genMakePath1(ObjResFile));
      writeln(file,"OBJ      = " + Objects + " $(RES)");
      writeln(file,"LINKOBJ  = " + LinkObjects + " $(RES)");
    } else {
      writeln(file,"OBJ      = " + Objects);
      writeln(file,"LINKOBJ  = " + LinkObjects);
    };
    libraryArguments.replace('\\', '/');
    writeln(file,"LIBS     = " + libraryArguments);
    cIncludeArguments.replace('\\', '/');
    writeln(file,"INCS     = " + cIncludeArguments);
    cppIncludeArguments.replace('\\', '/');
    writeln(file,"CXXINCS  = " + cppIncludeArguments);
    writeln(file,"BIN      = " + genMakePath1(extractRelativePath(mProject->makeFileName(), mProject->executable())));
    cppCompileArguments.replace('\\', '/');
    writeln(file,"CXXFLAGS = $(CXXINCS) " + cppCompileArguments);
    //writeln(file,"ENCODINGS = -finput-charset=utf-8 -fexec-charset='+GetSystemCharsetName);
    cCompileArguments.replace('\\', '/');
    writeln(file,"CFLAGS   = $(INCS) " + cCompileArguments);
    writeln(file, QString("RM       = ") + CLEAN_PROGRAM );
    if (mProject->options().usePrecompiledHeader){
        writeln(file,"PCH_H = " + mProject->options().precompiledHeader );
        writeln(file,"PCH = " + changeFileExt(mProject->options().precompiledHeader, GCH_EXT));
   }


    // This needs to be put in before the clean command.
    if (mProject->options().type == ProjectType::DynamicLib) {
        QString OutputFileDir = extractFilePath(mProject->executable());
        QString libOutputFile = includeTrailingPathDelimiter(OutputFileDir) + "lib" + extractFileName(mProject->executable());
        if (QFileInfo(libOutputFile).absoluteFilePath()
                == mProject->directory())
            libOutputFile = extractFileName(libOutputFile);
        else
            libOutputFile = extractRelativePath(mProject->makeFileName(), libOutputFile);
        writeln(file,"DEF      = " + genMakePath1(changeFileExt(libOutputFile, DEF_EXT)));
        writeln(file,"STATIC   = " + genMakePath1(changeFileExt(libOutputFile, LIB_EXT)));

    }
    writeln(file);
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
