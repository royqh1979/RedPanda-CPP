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
#include "../utils.h"

#include <QFileInfo>

GDBMIDebuggerClient::GDBMIDebuggerClient(Debugger *debugger, QObject *parent):
    DebuggerClient(debugger, parent)
{

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
    mInferiorRunning = false;
    mProcessExited = false;
    mErrorOccured = false;
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
                        mErrorOccured= true;
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
        if (mErrorOccured)
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
    if (mErrorOccured) {
        emit processError(mProcess->error());
    }
}

