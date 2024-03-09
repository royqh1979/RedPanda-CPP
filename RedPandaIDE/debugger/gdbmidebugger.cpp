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
#include "gdbmidebugger.h"
#include "../mainwindow.h"
#include "../editorlist.h"
#include "../utils.h"
#include "../systemconsts.h"
#include "../settings.h"

#include <QFileInfo>


const QRegularExpression GDBMIDebuggerClient::REGdbSourceLine("^(\\d)+\\s+in\\s+(.+)$");

GDBMIDebuggerClient::GDBMIDebuggerClient(Debugger *debugger, QObject *parent):
    DebuggerClient(debugger, parent)
{
    mProcess = std::make_shared<QProcess>();
    mAsyncUpdated = false;
}

void GDBMIDebuggerClient::postCommand(const QString &Command, const QString &Params,
                               DebugCommandSource Source)
{
    QMutexLocker locker(&mCmdQueueMutex);
    PDebugCommand pCmd = std::make_shared<DebugCommand>();
    pCmd->command = Command;
    pCmd->params = Params;
    pCmd->source = Source;
    mCmdQueue.enqueue(pCmd);
//    if (!mCmdRunning)
    //        runNextCmd();
}

void GDBMIDebuggerClient::registerInferiorStoppedCommand(const QString &Command, const QString &Params)
{
    QMutexLocker locker(&mCmdQueueMutex);
    PDebugCommand pCmd = std::make_shared<DebugCommand>();
    pCmd->command = Command;
    pCmd->params = Params;
    pCmd->source = DebugCommandSource::Other;
    mInferiorStoppedHookCommands.append(pCmd);
}

void GDBMIDebuggerClient::stopDebug()
{
    mStop = true;
}

void GDBMIDebuggerClient::run()
{
    mStop = false;
    bool errorOccured = false;
    mInferiorRunning = false;
    mProcessExited = false;
    QString cmd = debuggerPath();
//    QString arguments = "--annotate=2";
    QStringList arguments{"--interpret=mi", "--silent"};
    QString workingDir = QFileInfo(debuggerPath()).path();

    mProcess = std::make_shared<QProcess>();
    auto action = finally([&]{
        mProcess.reset();
    });
    mProcess->setProgram(cmd);
    mProcess->setArguments(arguments);
    mProcess->setProcessChannelMode(QProcess::MergedChannels);

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    QStringList pathAdded = binDirs();
    if (!path.isEmpty()) {
        path = pathAdded.join(PATH_SEPARATOR) + PATH_SEPARATOR + path;
    } else {
        path = pathAdded.join(PATH_SEPARATOR);
    }
    QString cmdDir = extractFileDir(cmd);
    if (!cmdDir.isEmpty()) {
        path = cmdDir + PATH_SEPARATOR + path;
    }
    env.insert("PATH",path);
    mProcess->setProcessEnvironment(env);

    mProcess->setWorkingDirectory(workingDir);

    connect(mProcess.get(), &QProcess::errorOccurred,
                    [&](){
                        errorOccured= true;
                    });
    QByteArray buffer;
    QByteArray readed;

    mProcess->start();
    mProcess->waitForStarted(5000);
    mStartSemaphore.release(1);
    while (true) {
        mProcess->waitForFinished(1);
        if (mProcess->state()!=QProcess::Running) {
            break;
        }
        if (mStop) {
            mProcess->terminate();
            mProcess->kill();
            break;
        }
        if (errorOccured)
            break;
        readed = mProcess->readAll();
        buffer += readed;

        if (readed.endsWith("\n")&& outputTerminated(buffer)) {
            processDebugOutput(buffer);
            buffer.clear();
            mCmdRunning = false;
            runNextCmd();
        } else if (!mCmdRunning && readed.isEmpty()){
            runNextCmd();
        } else if (readed.isEmpty()){
            msleep(1);
        }
    }
    if (errorOccured) {
        emit processFailed(mProcess->error());
    }
}
void GDBMIDebuggerClient::runNextCmd()
{
    QMutexLocker locker(&mCmdQueueMutex);

    if (mCurrentCmd) {
        DebugCommandSource commandSource = mCurrentCmd->source;
        mCurrentCmd=nullptr;
        if (commandSource!=DebugCommandSource::HeartBeat)
            emit cmdFinished();
    }
    if (mCmdQueue.isEmpty()) {
        if (debugger()->useDebugServer() && mInferiorRunning && !mAsyncUpdated) {
            mAsyncUpdated = true;
            QTimer::singleShot(50,this,&GDBMIDebuggerClient::asyncUpdate);
        }
        return;
    }

    PDebugCommand pCmd = mCmdQueue.dequeue();
    mCmdRunning = true;
    mCurrentCmd = pCmd;
    if (pCmd->source!=DebugCommandSource::HeartBeat)
        emit cmdStarted();

    QByteArray s;
    QByteArray params;
    s=pCmd->command.toLocal8Bit();
    if (!pCmd->params.isEmpty()) {
        params = pCmd->params.toLocal8Bit();
    }

    //clang compatibility
    if (debugger()->forceUTF8()) {
        params = pCmd->params.toUtf8();
    } else if (debugger()->debugInfosUsingUTF8() &&
               (pCmd->command=="-break-insert"
                || pCmd->command=="-var-create"
                || pCmd->command=="-data-read-memory"
                || pCmd->command=="-data-evaluate-expression"
                )) {
        params = pCmd->params.toUtf8();
    }
    if (pCmd->command == "-var-create") {
        //hack for variable creation,to easy remember var expression
        if (debugger()->debuggerType()==DebuggerType::LLDB_MI)
            params = " - * "+params;
        else
            params = " - @ "+params;
    } else if (pCmd->command == "-var-list-children") {
        //hack for list variable children,to easy remember var expression
        params = " --all-values \"" + params+'\"';
    }
    s+=" "+params;
    s+= "\n";
    if (mProcess->write(s)<0) {
        emit writeToDebugFailed();
    }

    if (pSettings->debugger().enableDebugConsole() ) {
        //update debug console
        if (pSettings->debugger().showDetailLog()
                && pCmd->source != DebugCommandSource::Console) {
            emit changeDebugConsoleLastLine(pCmd->command + ' ' + params);
        }
    }
}

QStringList GDBMIDebuggerClient::tokenize(const QString &s) const
{
    QStringList result;
    int tStart,tEnd;
    int i=0;
    while (i<s.length()) {
        QChar ch = s[i];
        if (ch == ' ' || ch == '\t'
                || ch == '\r'
                || ch == '\n') {
//            if (!current.isEmpty()) {
//                result.append(current);
//                current = "";
//            }
            i++;
            continue;
        } else if (ch == '\'') {
            tStart = i;
            i++; //skip \'
            while (i<s.length()) {
                if (s[i]=='\'') {
                    i++;
                    break;
                } else if (s[i] == '\\') {
                    i+=2;
                    continue;
                }
                i++;
            }
            tEnd = std::min(i,s.length());
            result.append(s.mid(tStart,tEnd-tStart));
        } else if (ch == '\"') {
            tStart = i;
            i++; //skip \'
            while (i<s.length()) {
                if (s[i]=='\"') {
                    i++;
                    break;
                } else if (s[i] == '\\') {
                    i+=2;
                    continue;
                }
                i++;
            }
            tEnd = std::min(i,s.length());
            result.append(s.mid(tStart,tEnd-tStart));
        } else if (ch == '<') {
            tStart = i;
            i++;
            while (i<s.length()) {
                if (s[i]=='>') {
                    i++;
                    break;
                }
                i++;
            }
            tEnd = std::min(i,s.length());
            result.append(s.mid(tStart,tEnd-tStart));
        } else if (ch == '(') {
            tStart = i;
            i++;
            while (i<s.length()) {
                if (s[i]==')') {
                    i++;
                    break;
                }
                i++;
            }
            tEnd = std::min(i,s.length());
            result.append(s.mid(tStart,tEnd-tStart));
        } else if (ch == '_' ||
                   ch == '.' ||
                   ch == '+' ||
                   ch == '-' ||
                   ch.isLetterOrNumber() ) {
            tStart = i;
            while (i<s.length()) {
                ch = s[i];
                if (!(ch == '_' ||
                     ch == '.' ||
                     ch == '+' ||
                     ch == '-' ||
                     ch.isLetterOrNumber() ))
                    break;
                i++;
            }
            tEnd = std::min(i,s.length());
            result.append(s.mid(tStart,tEnd-tStart));
        } else {
            result.append(s[i]);
            i++;
        }
    }
    return result;
}

bool GDBMIDebuggerClient::outputTerminated(const QByteArray &text) const
{
    QStringList lines = textToLines(QString::fromUtf8(text));
    foreach (const QString& line,lines) {
        if (line.trimmed() == "(gdb)")
            return true;
    }
    return false;
}

void GDBMIDebuggerClient::handleBreakpoint(const GDBMIResultParser::ParseObject& breakpoint)
{
    QString filename;
    // gdb use system encoding for file path
    if (debugger()->forceUTF8() || debugger()->debugInfosUsingUTF8())
        filename = breakpoint["fullname"].utf8PathValue();
    else
        filename = breakpoint["fullname"].pathValue();
    int line = breakpoint["line"].intValue();
    int number = breakpoint["number"].intValue();
    emit breakpointInfoGetted(filename, line , number);
}

void GDBMIDebuggerClient::handleFrame(const GDBMIResultParser::ParseValue &frame)
{
    if (frame.isValid()) {
        GDBMIResultParser::ParseObject frameObj = frame.object();
        bool ok;
        mCurrentAddress = frameObj["addr"].hexValue(ok);
        if (!ok)
            mCurrentAddress=0;
        mCurrentLine = frameObj["line"].intValue();
        if (debugger()->forceUTF8()
                || debugger()->debugInfosUsingUTF8())
            mCurrentFile = frameObj["fullname"].utf8PathValue();
        else
            mCurrentFile = frameObj["fullname"].pathValue();
        mCurrentFunc = frameObj["func"].value();
    }
}

void GDBMIDebuggerClient::handleStack(const QList<GDBMIResultParser::ParseValue> & stack)
{
    debugger()->backtraceModel()->clear();
    foreach (const GDBMIResultParser::ParseValue& frameValue, stack) {
        GDBMIResultParser::ParseObject frameObject = frameValue.object();
        PTrace trace = std::make_shared<Trace>();
        trace->funcname = frameObject["func"].value();
        if (debugger()->forceUTF8() || debugger()->debugInfosUsingUTF8())
            trace->filename = frameObject["fullname"].utf8PathValue();
        else
            trace->filename = frameObject["fullname"].pathValue();
        trace->line = frameObject["line"].intValue();
        trace->level = frameObject["level"].intValue(0);
        trace->address = frameObject["addr"].value();
        debugger()->backtraceModel()->addTrace(trace);
    }
}

void GDBMIDebuggerClient::handleLocalVariables(const QList<GDBMIResultParser::ParseValue> &variables)
{
    QStringList locals;
    foreach (const GDBMIResultParser::ParseValue& varValue, variables) {
        GDBMIResultParser::ParseObject varObject = varValue.object();
        QString name = QString(varObject["name"].value());
        QString value = QString(varObject["value"].value());
        locals.append(
                    QString("%1 = %2")
                    .arg(
                        name,
                        value
                ));
    }
    emit localsUpdated(locals);
}

void GDBMIDebuggerClient::handleEvaluation(const QString &value)
{
    emit evalUpdated(value);
}

void GDBMIDebuggerClient::handleMemory(const QList<GDBMIResultParser::ParseValue> &rows)
{
    QStringList memory;
    foreach (const GDBMIResultParser::ParseValue& row, rows) {
        GDBMIResultParser::ParseObject rowObject = row.object();
        QList<GDBMIResultParser::ParseValue> data = rowObject["data"].array();
        QStringList values;
        foreach (const GDBMIResultParser::ParseValue& val, data) {
            values.append(val.value());
        }
        memory.append(QString("%1 %2")
                            .arg(rowObject["addr"].value(),values.join(" ")));
    }
    emit memoryUpdated(memory);
}

void GDBMIDebuggerClient::handleRegisterNames(const QList<GDBMIResultParser::ParseValue> &names)
{
    QStringList nameList;
    foreach (const GDBMIResultParser::ParseValue& nameValue, names) {
//        QString text = nameValue.value().trimmed();
//        if (!text.isEmpty())
            nameList.append(nameValue.value());
    }
    emit registerNamesUpdated(nameList);
}

void GDBMIDebuggerClient::handleRegisterValue(const QList<GDBMIResultParser::ParseValue> &values)
{
    QHash<int,QString> result;
    foreach (const GDBMIResultParser::ParseValue& val, values) {
        GDBMIResultParser::ParseObject obj = val.object();
        int number = obj["number"].intValue();
        QString value = obj["value"].value();
        bool ok;
        long long intVal;
        intVal = value.toLongLong(&ok,10);
        if (ok) {
            value = QString("0x%1").arg(intVal,0,16);
        }
        result.insert(number,value);
    }
    emit registerValuesUpdated(result);
}

void GDBMIDebuggerClient::handleListVarChildren(const GDBMIResultParser::ParseObject &multiVars)
{
    if (!mCurrentCmd)
        return;
    QString parentName = mCurrentCmd->params;
    int parentNumChild = multiVars["numchild"].intValue(0);
    QList<GDBMIResultParser::ParseValue> children = multiVars["children"].array();
    bool hasMore = multiVars["has_more"].value()!="0";
    emit prepareVarChildren(parentName,parentNumChild,hasMore);
    foreach(const GDBMIResultParser::ParseValue& child, children) {
        GDBMIResultParser::ParseObject childObj = child.object();
        QString name = childObj["name"].value();
        QString exp = childObj["exp"].value();
        int numChild = childObj["numchild"].intValue(0);
        QString value = childObj["value"].value();
        QString type = childObj["type"].value();
        bool hasMore = childObj["has_more"].value() != "0";
        emit addVarChild(parentName,
                         name,
                         exp,
                         numChild,
                         value,
                         type,
                         hasMore);
    }
}


void GDBMIDebuggerClient::handleCreateVar(const GDBMIResultParser::ParseObject &multiVars)
{
    if (!mCurrentCmd)
        return;
    QString expression = mCurrentCmd->params;
    QString name = multiVars["name"].value();
    int numChild = multiVars["numchild"].intValue(0);
    QString value = multiVars["value"].value();
    QString type = multiVars["type"].value();
    bool hasMore = multiVars["has_more"].value() != "0";
    emit varCreated(expression,name,numChild,value,type,hasMore);
}

void GDBMIDebuggerClient::handleUpdateVarValue(const QList<GDBMIResultParser::ParseValue> &changes)
{
    foreach (const GDBMIResultParser::ParseValue& value, changes) {
        GDBMIResultParser::ParseObject obj = value.object();
        QString name = obj["name"].value();
        QString val = obj["value"].value();
        QString inScope = obj["in_scope"].value();
        bool typeChanged = (obj["type_changed"].value()=="true");
        QString newType = obj["new_type"].value();
        int newNumChildren = obj["new_num_children"].intValue(-1);
        bool hasMore = (obj["has_more"].value() == "1");
        emit varValueUpdated(name,val,inScope,typeChanged,newType,newNumChildren,
                             hasMore);
    }
    //todo: -var-list-children will freeze if the var is not correctly initialized
    //emit varsValueUpdated();
}

void GDBMIDebuggerClient::processConsoleOutput(const QByteArray& line)
{
    if (line.length()>3 && line.startsWith("~\"") && line.endsWith("\"")) {
        QByteArray s=line.mid(2,line.length()-3);
        QByteArray stringValue;
        const char *p=s.data();
        while (*p!=0) {
            if (*p=='\\' && *(p+1)!=0) {
                p++;
                switch (*p) {
                case '\'':
                    stringValue+=0x27;
                    p++;
                    break;
                case '"':
                    stringValue+=0x22;
                    p++;
                    break;
                case '?':
                    stringValue+=0x3f;
                    p++;
                    break;
                case '\\':
                    stringValue+=0x5c;
                    p++;
                    break;
                case 'a':
                    stringValue+=0x07;
                    p++;
                    break;
                case 'b':
                    stringValue+=0x08;
                    p++;
                    break;
                case 'f':
                    stringValue+=0x0c;
                    p++;
                    break;
                case 'n':
                    stringValue+=0x0a;
                    p++;
                    break;
                case 'r':
                    stringValue+=0x0d;
                    p++;
                    break;
                case 't':
                    stringValue+=0x09;
                    p++;
                    break;
                case 'v':
                    stringValue+=0x0b;
                    p++;
                    break;
                case '0':
                case '1':
                case '2':
                case '3':
                case '4':
                case '5':
                case '6':
                case '7':
                {
                    int i=0;
                    for (i=0;i<3;i++) {
                        if (*(p+i)<'0' || *(p+i)>'7')
                            break;
                    }
                    QByteArray numStr(p,i);
                    bool ok;
                    unsigned char ch = numStr.toInt(&ok,8);
                    stringValue+=ch;
                    p+=i;
                    break;
                }
                }
            } else {
                stringValue+=*p;
                p++;
            }
        }
        //mConsoleOutput.append(QString::fromLocal8Bit(stringValue));
        mConsoleOutput.append(QString::fromUtf8(stringValue));
    }
}

void GDBMIDebuggerClient::processLogOutput(const QByteArray &line)
{
    if (debugger()->debugInfosUsingUTF8() && line.endsWith(": No such file or directory.\n\"")) {
        QByteArray newLine = line;
        newLine[0]='~';
        int p=newLine.lastIndexOf(':');
        if (p>0) {
            newLine=newLine.left(p);
            //qDebug()<<newLine;
            processConsoleOutput(newLine);
        }
    }
}

void GDBMIDebuggerClient::processResult(const QByteArray &result)
{
    GDBMIResultParser parser;
    GDBMIResultType resultType;
    GDBMIResultParser::ParseObject multiValues;
    if (!mCurrentCmd)
        return;
    bool parseOk = parser.parse(result, mCurrentCmd->command, resultType,multiValues);
    if (!parseOk)
        return;
    switch(resultType) {
    case GDBMIResultType::BreakpointTable:
    case GDBMIResultType::Locals:
        break;
    case GDBMIResultType::Breakpoint:
        handleBreakpoint(multiValues["bkpt"].object());
        return;
    case GDBMIResultType::Frame:
        handleFrame(multiValues["frame"]);
        return;
    case GDBMIResultType::FrameStack:
        handleStack(multiValues["stack"].array());
        return;
    case GDBMIResultType::LocalVariables:
        handleLocalVariables(multiValues["variables"].array());
        return;
    case GDBMIResultType::Evaluation:
        handleEvaluation(multiValues["value"].value());
        return;
    case GDBMIResultType::Memory:
        handleMemory(multiValues["memory"].array());
        return;
    case GDBMIResultType::RegisterNames:
        handleRegisterNames(multiValues["register-names"].array());
        return;
    case GDBMIResultType::RegisterValues:
        handleRegisterValue(multiValues["register-values"].array());
        return;
    case GDBMIResultType::CreateVar:
        handleCreateVar(multiValues);
        return;
    case GDBMIResultType::ListVarChildren:
        handleListVarChildren(multiValues);
        return;
    case GDBMIResultType::UpdateVarValue:
        handleUpdateVarValue(multiValues["changelist"].array());
        return;
    default:
        return;
    }
}

void GDBMIDebuggerClient::processExecAsyncRecord(const QByteArray &line)
{
    QByteArray result;
    GDBMIResultParser::ParseObject multiValues;
    GDBMIResultParser parser;
    if (!parser.parseAsyncResult(line,result,multiValues))
        return;
    if (result == "running") {
        mInferiorRunning = true;
        mCurrentAddress=0;
        mCurrentFile.clear();
        mCurrentLine=-1;
        mCurrentFunc.clear();
        emit inferiorContinued();
        return;
    }
    if (result == "stopped") {
        mInferiorRunning = false;
        QByteArray reason = multiValues["reason"].value();
        if (reason == "exited") {
            //inferior exited, gdb should terminate too
            mProcessExited = true;
            return;
        }
        if (reason == "exited-normally") {
            //inferior exited, gdb should terminate too
            mProcessExited = true;
            return;
        }
        if (reason == "exited-signalled") {
            //inferior exited, gdb should terminate too
            mProcessExited = true;
            mSignalReceived = true;
            return;
        }
        mUpdateCPUInfo = true;
        handleFrame(multiValues["frame"]);
        if (reason == "signal-received") {
            mSignalReceived = true;
            mSignalName = multiValues["signal-name"].value();
            mSignalMeaning = multiValues["signal-meaning"].value();
        } else if (reason == "watchpoint-trigger") {
            QString var,oldVal,newVal;
            GDBMIResultParser::ParseValue wpt=multiValues["wpt"];
            if (wpt.isValid()) {
                GDBMIResultParser::ParseObject wptObj = wpt.object();
                var=wptObj["exp"].value();
            }
            GDBMIResultParser::ParseValue varValue=multiValues["value"];
            if (varValue.isValid()) {
                GDBMIResultParser::ParseObject valueObj = varValue.object();
                oldVal=valueObj["old"].value();
                newVal=valueObj["new"].value();
            }
            if (!var.isEmpty()) {
                emit watchpointHitted(var,oldVal,newVal);
            }
        }
        runInferiorStoppedHook();
        if (reason.isEmpty()) {
            QMutexLocker locker(&mCmdQueueMutex);
            foreach (const PDebugCommand& cmd, mCmdQueue) {
                //gdb-server connected, just ignore it
                if (cmd->command=="-exec-continue")
                    return;
            }
        }
        if (mCurrentCmd && mCurrentCmd->source == DebugCommandSource::Console)
            emit inferiorStopped(mCurrentFile, mCurrentLine, false);
        else
            emit inferiorStopped(mCurrentFile, mCurrentLine, true);
    }
}

void GDBMIDebuggerClient::processError(const QByteArray &errorLine)
{
    QString s = QString::fromLocal8Bit(errorLine);
    mConsoleOutput.append(s);
    int idx=s.indexOf(",msg=\"No symbol table is loaded");
    if (idx>0) {
        emit errorNoSymbolTable();
        return;
    }
}

void GDBMIDebuggerClient::processResultRecord(const QByteArray &line)
{
    if (line.startsWith("^exit")) {
        mProcessExited = true;
        return;
    }
    if (line.startsWith("^error")) {
        processError(line);
        return;
    }
    if (line.startsWith("^done")
            || line.startsWith("^running")) {
        int pos = line.indexOf(',');
        if (pos>=0) {
            QByteArray result = line.mid(pos+1);
            processResult(result);
        } else if (mCurrentCmd && !(mCurrentCmd->command.startsWith('-'))) {
            if (mCurrentCmd->command == "disas") {
                QStringList disOutput = mConsoleOutput;
                if (disOutput.length()>=3) {
                    disOutput.pop_back();
                    disOutput.pop_front();
                    disOutput.pop_front();
                }
                if (debugger()->debugInfosUsingUTF8()) {
                    QStringList newOutput;
                    foreach(const QString& s, disOutput) {
                        QString line = s;
                        if (!s.isEmpty() && s.front().isDigit()) {
                            QRegularExpressionMatch match = REGdbSourceLine.match(s);
//                            qDebug()<<s;
                            if (match.hasMatch()) {
                                bool isOk;
                                int lineno=match.captured(1).toInt(&isOk)-1;;
                                QString filename = match.captured(2).trimmed();
                                if (isOk && fileExists(filename)) {
                                    QStringList contents;
                                    if (mFileCache.contains(filename))
                                        contents = mFileCache.value(filename);
                                    else {
                                        if (!pMainWindow->editorList()->getContentFromOpenedEditor(filename,contents))
                                            contents = readFileToLines(filename);
                                        mFileCache[filename]=contents;
                                    }
                                    if (lineno>=0 && lineno<contents.size()) {
                                        line = QString("%1\t%2").arg(lineno+1).arg(contents[lineno]);
                                    }
                                }
                            }
                        }
                        newOutput.append(line);
                    }
                    disOutput=newOutput;
                }
                emit disassemblyUpdate(mCurrentFile,mCurrentFunc, disOutput);
            }
        }
        return ;
    }
    if (line.startsWith("^connected")) {
        //TODO: connected to remote target
        return;
    }
}

void GDBMIDebuggerClient::processDebugOutput(const QByteArray& debugOutput)
{
    // Only update once per update at most
    //WatchView.Items.BeginUpdate;

    emit parseStarted();

    mConsoleOutput.clear();
    mFullOutput.clear();

    mSignalReceived = false;
    mUpdateCPUInfo = false;
    mReceivedSFWarning = false;
    QList<QByteArray> lines = splitByteArrayToLines(debugOutput);

    for (int i=0;i<lines.count();i++) {
         QByteArray line = lines[i];
         if (pSettings->debugger().showDetailLog())
            mFullOutput.append(line);
         line = removeToken(line);
         if (line.isEmpty()) {
             continue;
         }
         switch (line[0]) {
         case '~': // console stream output
             processConsoleOutput(line);
             break;
         case '@': // target stream output
             break;
         case '&': // log stream output
             processLogOutput(line);
             break;
         case '^': // result record
             processResultRecord(line);
             break;
         case '*': // exec async output
             processExecAsyncRecord(line);
             break;
         case '+': // status async output
         case '=': // notify async output
             break;
         }
    }
    emit parseFinished();
    mConsoleOutput.clear();
    mFullOutput.clear();
}


QByteArray GDBMIDebuggerClient::removeToken(const QByteArray &line) const
{
    int p=0;
    while (p<line.length()) {
        QChar ch=line[p];
        if (ch<'0' || ch>'9') {
            break;
        }
        p++;
    }
    if (p<line.length())
        return line.mid(p);
    return line;
}

void GDBMIDebuggerClient::asyncUpdate()
{
    QMutexLocker locker(&mCmdQueueMutex);
    if (mCmdQueue.isEmpty()) {
        postCommand("-var-update"," --all-values *",DebugCommandSource::HeartBeat);
    }
    mAsyncUpdated = false;
}

const PDebugCommand &GDBMIDebuggerClient::currentCmd() const
{
    return mCurrentCmd;
}

void GDBMIDebuggerClient::interrupt()
{
    postCommand("-exec-interrupt", "", DebugCommandSource::Other);
}

void GDBMIDebuggerClient::refreshStackVariables()
{
    postCommand("-stack-list-variables", "--all-values", DebugCommandSource::Other);
}

void GDBMIDebuggerClient::readMemory(qulonglong startAddress, int rows, int cols)
{
    postCommand("-data-read-memory",QString("%1 x 1 %2 %3 ")
                .arg(startAddress)
                .arg(rows)
                .arg(cols),
                DebugCommandSource::Other
                );
}

void GDBMIDebuggerClient::setBreakpointCondition(PBreakpoint breakpoint)
{
    Q_ASSERT(breakpoint!=nullptr);
    QString condition = breakpoint->condition;
    if (condition.isEmpty()) {
        postCommand("-break-condition",
                    QString("%1").arg(breakpoint->number), DebugCommandSource::Other);
    } else {
        postCommand("-break-condition",
                    QString("%1 %2").arg(breakpoint->number).arg(condition), DebugCommandSource::Other);
    }
}

void GDBMIDebuggerClient::addWatchpoint(const QString &watchExp)
{
    if (!watchExp.isEmpty())
        postCommand("-break-watch", watchExp, DebugCommandSource::Other);
}

void GDBMIDebuggerClient::refreshWatchVar(PWatchVar var)
{

}

void GDBMIDebuggerClient::runInferiorStoppedHook()
{
    QMutexLocker locker(&mCmdQueueMutex);
    foreach (const PDebugCommand& cmd, mInferiorStoppedHookCommands) {
        mCmdQueue.push_front(cmd);
    }
}

void GDBMIDebuggerClient::clearCmdQueue()
{
    QMutexLocker locker(&mCmdQueueMutex);
    mCmdQueue.clear();
}
