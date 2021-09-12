#ifndef PROJECTCOMPILER_H
#define PROJECTCOMPILER_H

#include "compiler.h"
#include <QObject>

class Project;
class ProjectCompiler : public Compiler
{
    Q_OBJECT
public:
    ProjectCompiler(std::shared_ptr<Project> project, bool silent,bool onlyCheckSyntax);

private:
    void buildMakeFile();
    // Compiler interface
protected:
    bool prepareForCompile() override;
    QString pipedText() override;
    bool prepareForRebuild() override;
private:
    QString mMakefileName;
};

#endif // PROJECTCOMPILER_H
