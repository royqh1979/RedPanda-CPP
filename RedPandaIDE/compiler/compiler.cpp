#include "compiler.h"
#include "utils.h"
#include "compilermanager.h"
#include "../systemconsts.h"

#include <QFileInfo>
#include <QProcess>
#include <QString>
#include <QTextCodec>
#include <QTime>
#include <QApplication>
#include "../editor.h"
#include "../mainwindow.h"
#include "../editorlist.h"
#include "../parser/cppparser.h"
#include "../autolinkmanager.h"
#include "../platform.h"
#include "../project.h"

#define COMPILE_PROCESS_END "---//END//----"

Compiler::Compiler(const QString &filename, bool silent, bool onlyCheckSyntax):
    QThread(),
    mSilent(silent),
    mOnlyCheckSyntax(onlyCheckSyntax),
    mFilename(filename),
    mRebuild(false)
{

}

void Compiler::run()
{
    emit compileStarted();
    auto action = finally([this]{
        emit compileFinished();
    });
    try {
        if (!prepareForCompile()){
            return;
        }
        if (mRebuild && !prepareForRebuild()) {
            throw CompileError(tr("Clean before rebuild failed."));
        }
        mErrorCount = 0;
        mWarningCount = 0;
        QElapsedTimer timer;
        timer.start();
        runCommand(mCompiler, mArguments, mDirectory, pipedText());
        log("");
        log(tr("Compile Result:"));
        log("------------------");
        log(tr("- Errors: %1").arg(mErrorCount));
        log(tr("- Warnings: %1").arg(mWarningCount));
        if (!mOutputFile.isEmpty()) {
            log(tr("- Output Filename: %1").arg(mOutputFile));
            QLocale locale = QLocale::system();
            log(tr("- Output Size: %1").arg(locale.formattedDataSize(QFileInfo(mOutputFile).size())));
        }
        log(tr("- Compilation Time: %1 secs").arg(timer.elapsed() / 1000.0));
    } catch (CompileError e) {
        emit compileErrorOccured(e.reason());
    }

}

QString Compiler::getFileNameFromOutputLine(QString &line) {
    QString temp;
    line = line.trimmed();
    while (true) {
        int pos;
        if (line.length() > 2 && line[1]==':') { // full file path at start, ignore this ':'
            pos = line.indexOf(':',2);
        } else {
            pos = line.indexOf(':');
        }
        if ( pos < 0) {
            break;
        }
        temp = line.mid(0,pos);
        line.remove(0,pos+1);
        line=line.trimmed();
        if (temp.compare("<stdin>", Qt::CaseInsensitive)==0 ) {
            temp = mFilename;
            break;
        }

        if (QFileInfo(temp).fileName() == QLatin1String("ld.exe")) { // skip ld.exe
            continue;
        } else {
            break;
        }
    }
    return temp;
}

int Compiler::getLineNumberFromOutputLine(QString &line)
{
    line = line.trimmed();
    int pos = line.indexOf(':');
    int result=0;
    if (pos < 0) {
        pos = line.indexOf(',');
    }
    if (pos>=0) {
        result = line.midRef(0,pos).toInt();
        if (result > 0)
            line.remove(0,pos+1);
    }
    return result;
}

int Compiler::getColunmnFromOutputLine(QString &line)
{
    line = line.trimmed();
    int pos = line.indexOf(':');
    int result=0;
    if (pos < 0) {
        pos = line.indexOf(',');
    }
    if (pos>=0) {
        result = line.midRef(0,pos).toInt();
        if (result > 0)
            line.remove(0,pos+1);
    }
    return result;
}

CompileIssueType Compiler::getIssueTypeFromOutputLine(QString &line)
{
    CompileIssueType result = CompileIssueType::Other;
    line = line.trimmed();
    int pos = line.indexOf(':');
    if (pos>=0) {
        QString s=line.mid(0,pos);
        if (s == "error" || s == "fatal error") {
            mErrorCount += 1;
            line = tr("[Error] ")+line.mid(pos+1);
            result = CompileIssueType::Error;
        } else if (s == "warning") {
            mWarningCount += 1;
            line = tr("[Warning] ")+line.mid(pos+1);
            result = CompileIssueType::Warning;
        } else if (s == "info") {
            mWarningCount += 1;
            line = tr("[Info] ")+line.mid(pos+1);
            result = CompileIssueType::Info;
        } else if (s == "note") {
            mWarningCount += 1;
            line = tr("[Note] ")+line.mid(pos+1);
            result = CompileIssueType::Note;
        }
    }
    return result;
}

Settings::PCompilerSet Compiler::compilerSet()
{
    return pSettings->compilerSets().defaultSet();
}

void Compiler::processOutput(QString &line)
{
    if (line == COMPILE_PROCESS_END) {
        if (mLastIssue) {
            emit compileIssue(mLastIssue);
            mLastIssue.reset();
        }
        return;
    }
    QString inFilePrefix = QString("In file included from ");
    QString fromPrefix = QString("from ");
    PCompileIssue issue = std::make_shared<CompileIssue>();
    issue->type = CompileIssueType::Other;
    issue->endColumn = -1;
    if (line.startsWith(inFilePrefix)) {
        line.remove(0,inFilePrefix.length());
        issue->filename = getFileNameFromOutputLine(line);
        issue->line = getLineNumberFromOutputLine(line);
        if (issue->line > 0)
            issue->column = getColunmnFromOutputLine(line);
        issue->type = getIssueTypeFromOutputLine(line);
        issue->description = inFilePrefix + issue->filename;
        emit compileIssue(issue);
        return;
    } else if(line.startsWith(fromPrefix)) {
        line.remove(0,fromPrefix.length());
        issue->filename = getFileNameFromOutputLine(line);
        issue->line = getLineNumberFromOutputLine(line);
        if (issue->line > 0)
            issue->column = getColunmnFromOutputLine(line);
        issue->type = getIssueTypeFromOutputLine(line);
        issue->description = "                 from " + issue->filename;
        emit compileIssue(issue);
        return;
    }

    // Ignore code snippets that GCC produces
    // they always start with a space
    if (line.length()>0 && line[0] == ' ') {
        if (!mLastIssue)
            return;
        QString s = line.trimmed();
        if (s.startsWith('|') && s.indexOf('^')) {
            int pos = 0;
            while (pos < s.length()) {
                if (s[pos]=='^')
                    break;
                pos++;
            }
            if (pos<s.length()) {
                int i=pos+1;
                while (i<s.length()) {
                    if (s[i]!='~' && s[i]!='^')
                        break;
                    i++;
                }
                mLastIssue->endColumn = mLastIssue->column+i-pos;
                emit compileIssue(mLastIssue);
                mLastIssue.reset();
            }
        }
        return;
    }

    if (mLastIssue) {
        emit compileIssue(mLastIssue);
        mLastIssue.reset();
    }

    // assume regular main.cpp:line:col: message
    issue->filename = getFileNameFromOutputLine(line);
    issue->line = getLineNumberFromOutputLine(line);
    if (issue->line > 0) {
        issue->column = getColunmnFromOutputLine(line);
        issue->type = getIssueTypeFromOutputLine(line);
        if (issue->column<=0 && issue->type == CompileIssueType::Other) {
            issue->type = CompileIssueType::Error; //linkage error
            mErrorCount += 1;
        }
    } else {
        issue->column = -1;
        issue->type = getIssueTypeFromOutputLine(line);
    }
    issue->description = line.trimmed();
    if (issue->line<=0) {
        emit compileIssue(issue);
    } else
        mLastIssue = issue;
}

void Compiler::stopCompile()
{
    mStop = true;
}

QString Compiler::getCharsetArgument(const QByteArray& encoding)
{
    QString result;
    if (compilerSet()->autoAddCharsetParams() && encoding != ENCODING_ASCII
            && compilerSet()->compilerType()!="Clang") {
        QString encodingName;
        QString systemEncodingName=pCharsetInfoManager->getDefaultSystemEncoding();
        if (encoding == ENCODING_SYSTEM_DEFAULT) {
            encodingName = systemEncodingName;
        } else if (encoding == ENCODING_UTF8_BOM) {
            encodingName = "UTF-8";
        } else {
            encodingName = encoding;
        }
        result += QString(" -finput-charset=%1 -fexec-charset=%2")
                .arg(encodingName,systemEncodingName);
    }
    return result;
}

QString Compiler::getCCompileArguments(bool checkSyntax)
{
    QString result;
    if (checkSyntax) {
        result += " -fsyntax-only";
    }

    for (int i=0;i<compilerSet()->options().size();i++) {
        PCompilerOption pOption = compilerSet()->options()[i];
        // consider project specific options for the compiler, else global compiler options
        if (
                (mProject && (i < mProject->options().compilerOptions.length()))
                || (!mProject && (pOption->value > 0))) {
            int value;
            if (mProject) {
                value = Settings::CompilerSet::charToValue(mProject->options().compilerOptions[i]);
            } else {
                value = pOption->value;
            }
            if (value > 0 && pOption->isC) {
                if (checkSyntax && pOption->isLinker)
                    continue;
                if (pOption->choices.isEmpty()) {
                    result += " " + pOption->setting;
                } else if (value < pOption->choices.size()) {
                    QStringList nameValue=pOption->choices[value].split('=');
                    if (nameValue.count()==2) {
                        result += " " + pOption->setting + nameValue[1];
                    }
                }
            }
        }
    }

    if (compilerSet()->useCustomCompileParams() && !compilerSet()->customCompileParams().isEmpty()) {
        result += " "+ parseMacros(compilerSet()->customCompileParams());
    }

    if (mProject) {
        QString s = mProject->options().compilerCmd;
        if (!s.isEmpty()) {
            s.replace("_@@_", " ");
            result += " "+parseMacros(s);
        }
    }
    return result;
}

QString Compiler::getCppCompileArguments(bool checkSyntax)
{
    return getCCompileArguments(checkSyntax);
    QString result;
    if (checkSyntax) {
        result += " -fsyntax-only";
    }

    for (int i=0;i<compilerSet()->options().size();i++) {
        PCompilerOption pOption = compilerSet()->options()[i];
        // consider project specific options for the compiler, else global compiler options
        if (
                (mProject && (i < mProject->options().compilerOptions.length()))
                || (!mProject && (pOption->value > 0))) {
            int value;
            if (mProject) {
                value = Settings::CompilerSet::charToValue(mProject->options().compilerOptions[i]);
            } else {
                value = pOption->value;
            }
            if (value > 0 && pOption->isCpp) {
                if (checkSyntax && pOption->isLinker)
                    continue;
                if (pOption->choices.isEmpty()) {
                    result += " " + pOption->setting;
                } else if (value < pOption->choices.size()) {
                    QStringList nameValue=pOption->choices[value].split('=');
                    if (nameValue.count()==2) {
                        result += " " + pOption->setting + nameValue[1];
                    }
                }
            }
        }
    }

    if (compilerSet()->useCustomCompileParams() && !compilerSet()->customCompileParams().isEmpty()) {
        result += " "+ parseMacros(compilerSet()->customCompileParams());
    }
    if (mProject) {
        QString s = mProject->options().cppCompilerCmd;
        if (!s.isEmpty()) {
            s.replace("_@@_", " ");
            result += " "+parseMacros(s);
        }
    }
    return result;
}


QString Compiler::getCIncludeArguments()
{
    QString result;
    foreach (const QString& folder,compilerSet()->CIncludeDirs()) {
        result += QString(" -I\"%1\"").arg(folder);
    }
    return result;
}

QString Compiler::getProjectIncludeArguments()
{
    QString result;
    if (mProject) {
        foreach (const QString& folder,mProject->options().includes) {
            result += QString(" -I\"%1\"").arg(folder);
        }
//        result +=  QString(" -I\"%1\"").arg(extractFilePath(mProject->filename()));
    }
    return result;
}

QString Compiler::getCppIncludeArguments()
{
    QString result;
    foreach (const QString& folder,compilerSet()->CppIncludeDirs()) {
        result += QString(" -I\"%1\"").arg(folder);
    }
    return result;
}

QString Compiler::getLibraryArguments(FileType fileType)
{
    QString result;

    //Add libraries
    foreach (const QString& folder, compilerSet()->libDirs()) {
        result += QString(" -L\"%1\"").arg(folder);
    }

    //add libs added via project
    if (mProject) {
        foreach (const QString& folder, mProject->options().libs){
            result += QString(" -L\"%1\"").arg(folder);
        }
    }

    //Add auto links
    // is file and auto link enabled
    if (pSettings->editor().enableAutolink() && (fileType == FileType::CSource ||
            fileType == FileType::CppSource)){
        Editor* editor = pMainWindow->editorList()->getEditor();
        if (editor) {
            PCppParser parser = editor->parser();
            if (parser) {
                int waitCount = 0;
                //wait parsing ends, at most 1 second
                while(parser->parsing()) {
                    if (waitCount>0)
                        break;
                    waitCount++;
                    QThread::msleep(100);
                    QApplication *app=dynamic_cast<QApplication*>(
                                QApplication::instance());
                    app->processEvents();
                }
                QSet<QString> parsedFiles;
                result += parseFileIncludesForAutolink(
                            editor->filename(),
                            parsedFiles,
                            parser);
            }
        }

    }

    //add compiler set link options
    //options like "-static" must be added after "-lxxx"
    for (int i=0;i<compilerSet()->options().size();i++) {
        PCompilerOption pOption = compilerSet()->options()[i];
        // consider project specific options for the compiler, else global compiler options
        if (
                (mProject && (i < mProject->options().compilerOptions.length()))
                || (!mProject && (pOption->value > 0))) {
            int value;
            if (mProject) {
                value = Settings::CompilerSet::charToValue(mProject->options().compilerOptions[i]);
            } else {
                value = pOption->value;
            }
            if (value > 0 && pOption->isLinker) {
                if (pOption->choices.isEmpty()) {
                    result += " " + pOption->setting;
                } else if (value < pOption->choices.size()) {
                    QStringList nameValue=pOption->choices[value].split('=');
                    if (nameValue.count()==2) {
                        result += " " + pOption->setting + nameValue[1];
                    }
                }
            }
        }
    }

    // Add global compiler linker extras
    if (compilerSet()->useCustomLinkParams() && !compilerSet()->customLinkParams().isEmpty()) {
       result += " "+compilerSet()->customCompileParams();
    }

    if (mProject) {
        if (mProject->options().type == ProjectType::GUI) {
            result += " -mwindows";
        }

        if (!mProject->options().linkerCmd.isEmpty()) {
            QString s = mProject->options().linkerCmd;
            if (!s.isEmpty()) {
                s.replace("_@@_", " ");
                result += " "+s;
            }
        }
        if (mProject->options().staticLink)
            result += " -static";
    } else if (compilerSet()->staticLink()) {
        result += " -static";
    }
    return result;
}

QString Compiler::parseFileIncludesForAutolink(
        const QString &filename,
        QSet<QString>& parsedFiles,
        PCppParser& parser)
{
    QString result;
    QString baseName = extractFileName(filename);
    if (parsedFiles.contains(filename))
        return result;
    parsedFiles.insert(filename);
    PAutolink autolink = pAutolinkManager->getLink(baseName);
    if (autolink) {
        result += ' '+autolink->linkOption;
    }
    QSet<QString> includedFiles = parser->getFileDirectIncludes(filename);
    foreach (const QString& includeFilename, includedFiles) {
        result += parseFileIncludesForAutolink(
                    includeFilename,
                    parsedFiles,
                    parser);
    }
    return result;
}

void Compiler::runCommand(const QString &cmd, const QString  &arguments, const QString &workingDir, const QString& inputText)
{
    QProcess process;
    mStop = false;
    bool errorOccurred = false;
    process.setProgram(cmd);
    QString cmdDir = extractFileDir(cmd);
    if (!cmdDir.isEmpty()) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString path = env.value("PATH");
        if (path.isEmpty()) {
            path = cmdDir;
        } else {
            path = cmdDir + PATH_SEPARATOR + path;
        }
        env.insert("PATH",path);
        process.setProcessEnvironment(env);
    }
    process.setArguments(QProcess::splitCommand(arguments));
    process.setWorkingDirectory(workingDir);

    process.connect(&process, &QProcess::errorOccurred,
                    [&](){
                        errorOccurred= true;
                    });
    process.connect(&process, &QProcess::readyReadStandardError,[&process,this](){
        this->error(QString::fromLocal8Bit( process.readAllStandardError()));
    });
    process.connect(&process, &QProcess::readyReadStandardOutput,[&process,this](){
        this->log(QString::fromLocal8Bit( process.readAllStandardOutput()));
    });
    process.connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),[this](){
        this->error(COMPILE_PROCESS_END);
    });
    process.start();
    process.waitForStarted(5000);
    if (!inputText.isEmpty())
        process.write(inputText.toLocal8Bit());
    process.closeWriteChannel();
    while (true) {
        process.waitForFinished(1000);
        if (process.state()!=QProcess::Running) {
            break;
        }
        if (mStop) {
            process.terminate();
        }
        if (errorOccurred)
            break;
    }
    if (errorOccurred) {
        switch (process.error()) {
        case QProcess::FailedToStart:
            throw CompileError(tr("The compiler process for '%1' failed to start.").arg(mFilename));
            break;
        case QProcess::Crashed:
            if (!mStop)
                throw CompileError(tr("The compiler process crashed after starting successfully."));
            break;
        case QProcess::Timedout:
            throw CompileError(tr("The last waitFor...() function timed out."));
            break;
        case QProcess::WriteError:
            throw CompileError(tr("An error occurred when attempting to write to the compiler process."));
            break;
        case QProcess::ReadError:
            throw CompileError(tr("An error occurred when attempting to read from the compiler process."));
            break;
        default:
            throw CompileError(tr("An unknown error occurred."));
        }
    }
}

const std::shared_ptr<Project> &Compiler::project() const
{
    return mProject;
}

void Compiler::setProject(const std::shared_ptr<Project> &newProject)
{
    mProject = newProject;
}

bool Compiler::isRebuild() const
{
    return mRebuild;
}

void Compiler::setRebuild(bool isRebuild)
{
    mRebuild = isRebuild;
}

void Compiler::log(const QString &msg)
{
    emit compileOutput(msg);
}

void Compiler::error(const QString &msg)
{
    if (msg != COMPILE_PROCESS_END)
        emit compileOutput(msg);
    for (QString& s:msg.split("\n")) {
        if (!s.isEmpty())
            processOutput(s);
    }
}
