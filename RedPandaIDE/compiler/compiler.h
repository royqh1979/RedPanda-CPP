#ifndef COMPILER_H
#define COMPILER_H

#include <QThread>
#include "settings.h"
#include "../common.h"
#include "../parser/cppparser.h"

class Project;
class Compiler : public QThread
{
    Q_OBJECT
public:
    enum class TargetType {
        Invalid,
        cttNone,
        File,
        Project,
        StdIn
    };
    Compiler(const QString& filename, bool silent,bool onlyCheckSyntax);

    bool isRebuild() const;
    void setRebuild(bool isRebuild);

    const std::shared_ptr<Project> &project() const;
    void setProject(const std::shared_ptr<Project> &newProject);

signals:
    void compileStarted();
    void compileFinished();
    void compileOutput(const QString& msg);
    void compileIssue(PCompileIssue issue);
    void compileErrorOccured(const QString& reason);
public slots:
    void stopCompile();

protected:
    void run() override;
    void processOutput(QString& line);
    virtual QString getFileNameFromOutputLine(QString &line);
    virtual int getLineNumberFromOutputLine(QString &line);
    virtual int getColunmnFromOutputLine(QString &line);
    virtual CompileIssueType getIssueTypeFromOutputLine(QString &line);

protected:
    virtual Settings::PCompilerSet compilerSet();
    virtual bool prepareForCompile() = 0;
    virtual QString pipedText() = 0;
    virtual bool prepareForRebuild() = 0;
    virtual QString getCharsetArgument(const QByteArray& encoding);
    virtual QString getCCompileArguments(bool checkSyntax);
    virtual QString getCppCompileArguments(bool checkSyntax);
    virtual QString getCIncludeArguments();
    virtual QString getProjectIncludeArguments();
    virtual QString getCppIncludeArguments();
    virtual QString getLibraryArguments(FileType fileType);
    virtual QString parseFileIncludesForAutolink(
            const QString& filename,
            QSet<QString>& parsedFiles,
            PCppParser& parser);
    void log(const QString& msg);
    void error(const QString& msg);
    void runCommand(const QString& cmd, const QString& arguments, const QString& workingDir, const QString& inputText=QString());

protected:
    bool mSilent;
    bool mOnlyCheckSyntax;
    QString mCompiler;
    QString mArguments;
    QString mOutputFile;
    int mErrorCount;
    int mWarningCount;
    PCompileIssue mLastIssue;
    QString mFilename;
    QString mDirectory;
    bool mRebuild;
    std::shared_ptr<Project> mProject;

private:
    bool mStop;
};


#endif // COMPILER_H
