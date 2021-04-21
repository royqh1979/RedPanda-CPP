#include "executablerunner.h"

#include <QProcess>
#include <windows.h>
#include <QDebug>

ExecutableRunner::ExecutableRunner(const QString &filename, const QString &arguments, const QString &workDir):
    QThread(),
    mFilename(filename),
    mArguments(arguments),
    mWorkDir(workDir),
    mStop(false)
{

}

void ExecutableRunner::stop()
{
    mStop = true;
}

void ExecutableRunner::run()
{
    emit started();
    QProcess process;
    mStop = false;
    process.setProgram(mFilename);
    process.setArguments(QProcess::splitCommand(mArguments));
    process.setWorkingDirectory(mWorkDir);
    process.setCreateProcessArgumentsModifier([](QProcess::CreateProcessArguments * args){
        args->flags |= CREATE_NEW_CONSOLE;
        args->startupInfo -> dwFlags &= ~STARTF_USESTDHANDLES;
    });
    qDebug() << mFilename;
    qDebug() << QProcess::splitCommand(mArguments);
    process.start();
    process.closeWriteChannel();
    process.waitForStarted(5000);
    while (true) {
        process.waitForFinished(1000);
        if (process.state()!=QProcess::Running) {
            break;
        }
        if (mStop) {
            process.kill();
            break;
        }
    }
    emit terminated();
    this->deleteLater();
}
