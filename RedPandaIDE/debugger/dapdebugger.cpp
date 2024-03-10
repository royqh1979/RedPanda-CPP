#include "dapdebugger.h"
#include "../utils.h"
#include "dapprotocol.h"
#include "../systemconsts.h"

#include <QFileInfo>

DAPDebuggerClient::DAPDebuggerClient(Debugger *debugger, QObject *parent):
    DebuggerClient{debugger, parent}
{

}

void DAPDebuggerClient::run()
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

        // if (readed.endsWith("\n")&& outputTerminated(buffer)) {
        //     processDebugOutput(buffer);
        //     buffer.clear();
        //     mCmdRunning = false;
        //     runNextCmd();
        // } else if (!mCmdRunning && readed.isEmpty()){
        //     runNextCmd();
        // } else if (readed.isEmpty()){
        //     msleep(1);
        // }
    }
    if (errorOccured) {
        emit processFailed(mProcess->error());
    }
}

DebuggerType DAPDebuggerClient::clientType()
{
    return DebuggerType::DAP;
}
