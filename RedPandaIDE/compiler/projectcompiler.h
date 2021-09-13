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
    void createStandardMakeFile();
    void createStaticMakeFile();
    void createDynamicMakeFile();
    void newMakeFile(QFile& file);
    void writeMakeHeader(QFile& file);
    void writeMakeDefines(QFile& file);
    void writeMakeTarget(QFile& file);
    void writeMakeIncludes(QFile& file);
    void writeMakeClean(QFile& file);
    void writeMakeObjFilesRules(QFile& file);
    void writeln(QFile& file, const QString& s="");
    // Compiler interface
protected:
    bool prepareForCompile() override;
    QString pipedText() override;
    bool prepareForRebuild() override;
};

#endif // PROJECTCOMPILER_H
