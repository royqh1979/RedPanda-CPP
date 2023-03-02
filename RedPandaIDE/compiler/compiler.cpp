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
#include "qt_utils/charsetinfo.h"
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
        emit compileFinished(mFilename);
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
        for(int i=0;i<mExtraArgumentsList.count();i++) {
            runCommand(mExtraCompilersList[i],mExtraArgumentsList[i],mDirectory, pipedText());
        }
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
            return temp;
        } else if (temp.compare("{standard input}", Qt::CaseInsensitive)==0 ) {
            temp = mFilename;
            return temp;
        }

        QFileInfo fileInfo(temp);
        if (fileInfo.fileName() == QLatin1String("ld.exe")) { // skip ld.exe
            continue;
        } else if (fileInfo.fileName() == QLatin1String("make")) { // skip make.exe
            continue;
        } else if (fileInfo.fileName() == QLatin1String("mingw32-make")) { // skip mingw32-make.exe
            continue;
        } else if (fileInfo.suffix()=="o") { // skip obj file
            continue;
        } else {
            break;
        }
    }
    if (!mDirectory.isEmpty()) {
        QFileInfo info(temp);
        return info.isRelative()?generateAbsolutePath(mDirectory,temp):cleanPath(temp);
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
    } else {
        result = line.toInt();
        if (result > 0)
            line="";
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
    if (mProject) {
        int index = mProject->options().compilerSet;
        Settings::PCompilerSet set = pSettings->compilerSets().getSet(index);
        if (set)
            return set;
    }
    return pSettings->compilerSets().defaultSet();
}

QByteArray Compiler::pipedText()
{
    return QByteArray();
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
    if (line.startsWith(">>>"))
        line.remove(0,3);
    QString referencePrefix = QString(" referenced by ");
    if(mLastIssue && line.startsWith(referencePrefix)) {
            line.remove(0,referencePrefix.length());
            mLastIssue->filename = getFileNameFromOutputLine(line);
            //qDebug()<<line;
            mLastIssue->line = getLineNumberFromOutputLine(line);
            emit compileIssue(mLastIssue);
            mLastIssue.reset();
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
    if (issue->line<=0 && (issue->filename=="ld" || issue->filename=="lld")) {
        mLastIssue = issue;
    } else if (issue->line<=0) {
        emit compileIssue(issue);
    } else
        mLastIssue = issue;
}

void Compiler::stopCompile()
{
    mStop = true;
}

QString Compiler::getCharsetArgument(const QByteArray& encoding,FileType fileType, bool checkSyntax)
{
    QString result;
    bool forceExecUTF8=false;
    // test if force utf8 from autolink infos
    if ((fileType == FileType::CSource ||
            fileType == FileType::CppSource) && pSettings->editor().enableAutolink() ){
        Editor* editor = pMainWindow->editorList()->getEditor();
        if (editor) {
            PCppParser parser = editor->parser();
            if (parser) {
                int waitCount = 0;
                //wait parsing ends, at most 1 second
                while(parser->parsing()) {
                    if (waitCount>10)
                        break;
                    waitCount++;
                    QThread::msleep(100);
                    QApplication *app=dynamic_cast<QApplication*>(
                                QApplication::instance());
                    app->processEvents();
                }
                QSet<QString> parsedFiles;
                forceExecUTF8 = parseForceUTF8ForAutolink(
                            editor->filename(),
                            parsedFiles,
                            parser);
            }
        }

    }
    if ((forceExecUTF8 || compilerSet()->autoAddCharsetParams()) && encoding != ENCODING_ASCII
            && compilerSet()->compilerType()!=CompilerType::Clang) {
        QString encodingName;
        QString execEncodingName;
        QString compilerSetExecCharset = compilerSet()->execCharset();
        QString systemEncodingName=pCharsetInfoManager->getDefaultSystemEncoding();
        if (encoding == ENCODING_SYSTEM_DEFAULT) {
            encodingName = systemEncodingName;
        } else if (encoding == ENCODING_UTF8_BOM) {
            encodingName = "UTF-8";
        } else if (encoding == ENCODING_UTF16_BOM) {
            encodingName = "UTF-16";
        } else if (encoding == ENCODING_UTF32_BOM) {
            encodingName = "UTF-32";
        } else {
            encodingName = encoding;
        }
        if (forceExecUTF8) {
            execEncodingName = "UTF-8";
        } else if (compilerSetExecCharset == ENCODING_SYSTEM_DEFAULT || compilerSetExecCharset.isEmpty()) {
            execEncodingName = systemEncodingName;
        } else {
            execEncodingName = compilerSetExecCharset;
        }
        //qDebug()<<encodingName<<execEncodingName;
        if (checkSyntax) {
            result += QString(" -finput-charset=%1")
                    .arg(encodingName);
        } else if (encodingName!=execEncodingName) {
            result += QString(" -finput-charset=%1 -fexec-charset=%2")
                    .arg(encodingName, execEncodingName);
        }
    }
    return result;
}

QString Compiler::getCCompileArguments(bool checkSyntax)
{
    QString result;
    if (checkSyntax) {
        result += " -fsyntax-only";
    }

    QMap<QString, QString> compileOptions;
    if (mProject && !mProject->options().compilerOptions.isEmpty()) {
        compileOptions = mProject->options().compilerOptions;
    } else {
        compileOptions = compilerSet()->compileOptions();
    }
    foreach (const QString& key, compileOptions.keys()) {
        if (compileOptions[key]=="")
            continue;
        PCompilerOption pOption = CompilerInfoManager::getCompilerOption(compilerSet()->compilerType(), key);
        if (pOption && pOption->isC && !pOption->isLinker) {
            if (pOption->choices.isEmpty())
                result += " " + pOption->setting;
            else {
                result += " " + pOption->setting + compileOptions[key];
            }
        }
    }

    if (compilerSet()->useCustomCompileParams() && !compilerSet()->customCompileParams().isEmpty()) {
        QStringList params = textToLines(compilerSet()->customCompileParams());
        foreach(const QString& param, params)
            result += " "+ parseMacros(param);
    }

    if (mProject) {
        QString s = mProject->options().compilerCmd;
        if (!s.isEmpty()) {
            s.replace("_@@_", " ");
            QStringList params = textToLines(s);
            foreach(const QString& param, params)
                result += " "+ parseMacros(param);
        }
    }
    return result;
}

QString Compiler::getCppCompileArguments(bool checkSyntax)
{
    QString result;
    if (checkSyntax) {
        result += " -fsyntax-only";
    }
    QMap<QString, QString> compileOptions;
    if (mProject && !mProject->options().compilerOptions.isEmpty()) {
        compileOptions = mProject->options().compilerOptions;
    } else {
        compileOptions = compilerSet()->compileOptions();
    }
    foreach (const QString& key, compileOptions.keys()) {
        if (compileOptions[key]=="")
            continue;
        PCompilerOption pOption = CompilerInfoManager::getCompilerOption(compilerSet()->compilerType(), key);
        if (pOption && pOption->isCpp && !pOption->isLinker) {
            if (pOption->choices.isEmpty())
                result += " " + pOption->setting;
            else
                result += " " + pOption->setting + compileOptions[key];
        }
    }
    if (compilerSet()->useCustomCompileParams() && !compilerSet()->customCompileParams().isEmpty()) {
        QStringList params = textToLines(compilerSet()->customCompileParams());
        foreach(const QString& param, params)
            result += " "+ parseMacros(param);
    }
    if (mProject) {
        QString s = mProject->options().cppCompilerCmd;
        if (!s.isEmpty()) {
            s.replace("_@@_", " ");
            QStringList params = textToLines(s);
            foreach(const QString& param, params)
                result += " "+ parseMacros(param);
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
        foreach (const QString& folder,mProject->options().includeDirs) {
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
        foreach (const QString& folder, mProject->options().libDirs){
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
                    if (waitCount>10)
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
    QMap<QString, QString> compileOptions;
    if (mProject && !mProject->options().compilerOptions.isEmpty()) {
        compileOptions = mProject->options().compilerOptions;
    } else {
        compileOptions = compilerSet()->compileOptions();
    }
    foreach (const QString& key, compileOptions.keys()) {
        if (compileOptions[key]=="")
            continue;
        PCompilerOption pOption = CompilerInfoManager::getCompilerOption(compilerSet()->compilerType(), key);
        if (pOption && pOption->isLinker) {
            if (pOption->choices.isEmpty())
                result += " " + pOption->setting;
            else
                result += " " + pOption->setting + compileOptions[key];
        }
    }

    // Add global compiler linker extras
    if (compilerSet()->useCustomLinkParams() && !compilerSet()->customLinkParams().isEmpty()) {
       QStringList params = textToLines(compilerSet()->customLinkParams());
       if (!params.isEmpty()) {
            foreach(const QString& param, params)
                result += " " + param;
       }
    }

    if (mProject) {
        if (mProject->options().type == ProjectType::GUI) {
            result += " -mwindows";
        }

        if (!mProject->options().linkerCmd.isEmpty()) {
            QString s = mProject->options().linkerCmd;
            if (!s.isEmpty()) {
                s.replace("_@@_", " ");
                QStringList params = textToLines(s);
                if (!params.isEmpty()) {
                     foreach(const QString& param, params)
                         result += " " + param;
                }
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
    if (parsedFiles.contains(filename))
        return result;
    parsedFiles.insert(filename);
    PAutolink autolink = pAutolinkManager->getLink(filename);
    if (autolink) {
        result += ' '+autolink->linkOption;
    }
    QStringList includedFiles = parser->getFileDirectIncludes(filename);
//    log(QString("File %1 included:").arg(filename));
//    for (int i=includedFiles.size()-1;i>=0;i--) {
//        QString includeFilename = includedFiles[i];
//        log(includeFilename);
//    }

    for (int i=includedFiles.size()-1;i>=0;i--) {
        QString includeFilename = includedFiles[i];
        result += parseFileIncludesForAutolink(
                    includeFilename,
                    parsedFiles,
                    parser);
    }
    return result;
}

bool Compiler::parseForceUTF8ForAutolink(const QString &filename, QSet<QString> &parsedFiles, PCppParser &parser)
{
    bool result;
    if (parsedFiles.contains(filename))
        return false;
    parsedFiles.insert(filename);
    PAutolink autolink = pAutolinkManager->getLink(filename);
    if (autolink && autolink->execUseUTF8) {
        return true;
    }
    QStringList includedFiles = parser->getFileDirectIncludes(filename);
//    log(QString("File %1 included:").arg(filename));
//    for (int i=includedFiles.size()-1;i>=0;i--) {
//        QString includeFilename = includedFiles[i];
//        log(includeFilename);
//    }

    for (int i=includedFiles.size()-1;i>=0;i--) {
        QString includeFilename = includedFiles[i];
        result = parseForceUTF8ForAutolink(
                    includeFilename,
                    parsedFiles,
                    parser);
        if (result)
            return true;
    }
    return false;
}

void Compiler::runCommand(const QString &cmd, const QString  &arguments, const QString &workingDir, const QByteArray& inputText)
{
    QProcess process;
    mStop = false;
    bool errorOccurred = false;
    process.setProgram(cmd);
    QString cmdDir = extractFileDir(cmd);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!cmdDir.isEmpty()) {
        QString path = env.value("PATH");
        if (path.isEmpty()) {
            path = cmdDir;
        } else {
            path = cmdDir + PATH_SEPARATOR + path;
        }
        env.insert("PATH",path);
    }
    //env.insert("LANG","en");
    env.insert("LDFLAGS","-Wl,--stack,12582912");
    env.insert("CFLAGS","");
    env.insert("CXXFLAGS","");
    process.setProcessEnvironment(env);
    process.setArguments(splitProcessCommand(arguments));
    process.setWorkingDirectory(workingDir);

    process.connect(&process, &QProcess::errorOccurred,
                    [&](){
                        errorOccurred= true;
                    });
    process.connect(&process, &QProcess::readyReadStandardError,[&process,this](){
        if (compilerSet()->compilerType() == CompilerType::Clang)
            this->error(QString::fromUtf8(process.readAllStandardError()));
        else
            this->error(QString::fromLocal8Bit( process.readAllStandardError()));
    });
    process.connect(&process, &QProcess::readyReadStandardOutput,[&process,this](){
        if (compilerSet()->compilerType() == CompilerType::Clang)
            this->log(QString::fromUtf8(process.readAllStandardOutput()));
        else
            this->log(QString::fromLocal8Bit( process.readAllStandardOutput()));
    });
    process.connect(&process, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),[this](){
        this->error(COMPILE_PROCESS_END);
    });
    process.start();
    process.waitForStarted(5000);
    if (!inputText.isEmpty()) {
        process.write(inputText);
        process.waitForFinished(0);
    }
    bool writeChannelClosed = false;
    while (true) {
        if (process.bytesToWrite()==0 && !writeChannelClosed ) {
            writeChannelClosed=true;
            process.closeWriteChannel();
        }
        process.waitForFinished(100);
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
