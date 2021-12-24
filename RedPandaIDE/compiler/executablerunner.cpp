#include "executablerunner.h"

#include <QDebug>
#include "compilermanager.h"
#include "../settings.h"
#include "../systemconsts.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif


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
        mProcess.reset();
        setPausing(false);
        emit terminated();
    });
    mStop = false;
    bool errorOccurred = false;

    mProcess = std::make_shared<QProcess>();
    mProcess->setProgram(mFilename);
    mProcess->setArguments(QProcess::splitCommand(mArguments));
    mProcess->setWorkingDirectory(mWorkDir);
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
    mProcess->setProcessEnvironment(env);
    connect(
                mProcess.get(), &QProcess::errorOccurred,
                [&errorOccurred](){
        errorOccurred= true;
    });
#ifdef Q_OS_WIN
    mProcess->setCreateProcessArgumentsModifier([this](QProcess::CreateProcessArguments * args){
        if (mStartConsole) {
            args->flags |=  CREATE_NEW_CONSOLE;
            args->flags &= ~CREATE_NO_WINDOW;
        }
        if (!mRedirectInput) {
            args->startupInfo -> dwFlags &= ~STARTF_USESTDHANDLES;
        }
    });
    HANDLE hSharedMemory=INVALID_HANDLE_VALUE;
    int BUF_SIZE=1024;
    char* pBuf=nullptr;
    if (mStartConsole) {
        hSharedMemory = CreateFileMappingA(
                INVALID_HANDLE_VALUE,
                NULL,
                PAGE_READWRITE,
                0,
                100,
                "RED_PANDA_IDE_CONSOLE_PAUSER20211223"
                );
        if (hSharedMemory != NULL)
        {
            pBuf = (char*) MapViewOfFile(hSharedMemory,   // handle to map object
                                 FILE_MAP_ALL_ACCESS, // read/write permission
                                 0,
                                 0,
                                 BUF_SIZE);
            if (pBuf) {
                pBuf[0]=0;
            }
        }
    }
#endif
//    if (!redirectInput()) {
//        process.closeWriteChannel();
//    }
    mProcess->start();
    mProcess->waitForStarted(5000);
    if (mProcess->state()==QProcess::Running && redirectInput()) {
        mProcess->write(readFileToByteArray(redirectInputFilename()));
        mProcess->closeWriteChannel();
    }

    while (true) {
        mProcess->waitForFinished(1000);
        if (mProcess->state()!=QProcess::Running) {
            break;
        }
        if (mStop) {
            mProcess->closeReadChannel(QProcess::StandardOutput);
            mProcess->closeReadChannel(QProcess::StandardError);
            mProcess->closeWriteChannel();
            mProcess->terminate();
            if (mProcess->waitForFinished(1000)) {
                break;
            }
            for (int i=0;i<10;i++) {
                mProcess->kill();
                if (mProcess->waitForFinished(500)) {
                    break;
                }
            }
            break;
        }
#ifdef Q_OS_WIN
        if (mStartConsole && !mPausing && pBuf) {
            if (strncmp(pBuf,"FINISHED",sizeof("FINISHED"))==0) {
                setPausing(true);
                emit pausingForFinish();
            }
        }
#endif
        if (errorOccurred)
            break;
    }
#ifdef Q_OS_WIN
    if (pBuf)
        UnmapViewOfFile(pBuf);
    if (hSharedMemory!=INVALID_HANDLE_VALUE)
        CloseHandle(hSharedMemory);
#endif
    if (errorOccurred) {
        //qDebug()<<"process error:"<<process.error();
        switch (mProcess->error()) {
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

void ExecutableRunner::doStop()
{
    std::shared_ptr<QProcess> process = mProcess;
    if (process) {
//        qDebug()<<"??1";
//        process->closeReadChannel(QProcess::StandardOutput);
//        process->closeReadChannel(QProcess::StandardError);
//        process->closeWriteChannel();
//        qDebug()<<"??2";
//    #ifdef Q_OS_WIN
//        if (!mStartConsole) {
//            qDebug()<<"??3";
//            process->terminate();
//            qDebug()<<"??4";
//            if (process->waitForFinished(1000)) {
//                return;
//            }
//        }
//    #else
//        process->terminate();
//        if (process->waitForFinished(1000)) {
//            break;
//        }
//    #endif
//        for (int i=0;i<10;i++) {
//            qDebug()<<"??5";
//            process->kill();
//            qDebug()<<"??6";
//            if (process->waitForFinished(100)) {
//                break;
//            }
//        }
    }
}
