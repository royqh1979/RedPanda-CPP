#include "executablerunner.h"

#include <QProcess>
#include <windows.h>
#include <QDebug>
#include "compilermanager.h"
#include "../settings.h"
#include "../systemconsts.h"

ExecutableRunner::ExecutableRunner(const QString &filename, const QString &arguments, const QString &workDir
                                   ,QObject* parent):
    Runner(filename,arguments,workDir,parent),
    mRedirectInput(false),
    mStartConsole(false)
{

}

bool ExecutableRunner::startConsole() const
{
    return mStartConsole;
}

void ExecutableRunner::setStartConsole(bool newStartConsole)
{
    mStartConsole = newStartConsole;
}

bool ExecutableRunner::redirectInput() const
{
    return mRedirectInput;
}

void ExecutableRunner::setRedirectInput(bool isRedirect)
{
    mRedirectInput = isRedirect;
}

const QString &ExecutableRunner::redirectInputFilename() const
{
    return mRedirectInputFilename;
}

void ExecutableRunner::setRedirectInputFilename(const QString &newDataFile)
{
    mRedirectInputFilename = newDataFile;
}

void ExecutableRunner::run()
{
    emit started();
    auto action = finally([this]{
        emit terminated();
    });
    QProcess process;
    mStop = false;
    bool errorOccurred = false;

    process.setProgram(mFilename);
    process.setArguments(QProcess::splitCommand(mArguments));
    process.setWorkingDirectory(mWorkDir);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    QStringList pathAdded;
    if (pSettings->compilerSets().defaultSet()) {
        foreach(const QString& dir, pSettings->compilerSets().defaultSet()->binDirs()) {
            pathAdded.append(dir);
        }
    }
    pathAdded.append(pSettings->dirs().app());
    if (!path.isEmpty()) {
        path+= PATH_SEPARATOR + pathAdded.join(PATH_SEPARATOR);
    } else {
        path = pathAdded.join(PATH_SEPARATOR);
    }
    env.insert("PATH",path);
    process.setProcessEnvironment(env);
    process.setCreateProcessArgumentsModifier([this](QProcess::CreateProcessArguments * args){
        if (mStartConsole) {
            args->flags |= CREATE_NEW_CONSOLE;
        }
        if (!mRedirectInput) {
            args->startupInfo -> dwFlags &= ~STARTF_USESTDHANDLES;
        }
    });
    process.connect(&process, &QProcess::errorOccurred,
                    [&](){
                        errorOccurred= true;
                    });
//    if (!redirectInput()) {
//        process.closeWriteChannel();
//    }
    process.start();
    process.waitForStarted(5000);
    if (process.state()==QProcess::Running && redirectInput()) {
        process.write(ReadFileToByteArray(redirectInputFilename()));
        process.closeWriteChannel();
    }
    while (true) {
        process.waitForFinished(1000);
        if (process.state()!=QProcess::Running) {
            break;
        }
        if (mStop) {
            process.closeReadChannel(QProcess::StandardOutput);
            process.closeReadChannel(QProcess::StandardError);
            process.closeWriteChannel();
            process.terminate();
            process.kill();
            break;
        }
        if (errorOccurred)
            break;
    }
    if (errorOccurred) {
        //qDebug()<<"process error:"<<process.error();
        switch (process.error()) {
        case QProcess::FailedToStart:
            emit runErrorOccurred(tr("The runner process '%1' failed to start.").arg(mFilename));
            break;
//        case QProcess::Crashed:
//            if (!mStop)
//                emit runErrorOccurred(tr("The runner process crashed after starting successfully."));
//            break;
        case QProcess::Timedout:
            emit runErrorOccurred(tr("The last waitFor...() function timed out."));
            break;
        case QProcess::WriteError:
            emit runErrorOccurred(tr("An error occurred when attempting to write to the runner process."));
            break;
        case QProcess::ReadError:
            emit runErrorOccurred(tr("An error occurred when attempting to read from the runner process."));
            break;
        default:
            break;
        }
    }
}
