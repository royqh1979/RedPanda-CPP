#include "projectcompiler.h"
#include "project.h"

ProjectCompiler::ProjectCompiler(std::shared_ptr<Project> project, bool silent, bool onlyCheckSyntax):
    Compiler("",silent,onlyCheckSyntax)
{
    setProject(project);
}

void ProjectCompiler::buildMakeFile()
{
    if (mProject->options().useCustomMakefile)
        mMakefileName = mProject->options().customMakefile;
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

QString ProjectCompiler::pipedText()
{
    return QString();
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

    mCompiler = QString("%1 -f \"%2\" all").arg(compilerSet()->make(),
                                                mMakefileName);
}
