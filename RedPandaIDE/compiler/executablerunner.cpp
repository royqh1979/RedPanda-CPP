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
#include "executablerunner.h"

#include <QDebug>
#include "compilermanager.h"
#include "../settings.h"
#include "../systemconsts.h"
#ifdef Q_OS_WIN
#include <QUuid>
#include <windows.h>
#else
#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#endif


ExecutableRunner::ExecutableRunner(const QString &filename, const QStringList &arguments, const QString &workDir
                                   ,QObject* parent):
    Runner(filename,arguments,workDir,parent),
    mRedirectInput(false),
    mStartConsole(false),
    mQuitSemaphore(0)
{
    setWaitForFinishTime(1000);
}

bool ExecutableRunner::startConsole() const
{
    return mStartConsole;
}

void ExecutableRunner::setStartConsole(bool newStartConsole)
{
    mStartConsole = newStartConsole;
}

const QString &ExecutableRunner::shareMemoryId() const
{
    return mShareMemoryId;
}

void ExecutableRunner::setShareMemoryId(const QString &newShareMemoryId)
{
    mShareMemoryId = newShareMemoryId;
}

const QStringList &ExecutableRunner::binDirs() const
{
    return mBinDirs;
}

void ExecutableRunner::addBinDirs(const QStringList &binDirs)
{
    mBinDirs.append(binDirs);
}

void ExecutableRunner::addBinDir(const QString &binDir)
{
    mBinDirs.append(binDir);
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
    mProcess->setArguments(mArguments);
    //qDebug()<<splitProcessCommand(mArguments);
    mProcess->setWorkingDirectory(mWorkDir);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    QStringList pathAdded = mBinDirs;
    if (!path.isEmpty()) {
        path = pathAdded.join(PATH_SEPARATOR) + PATH_SEPARATOR + path;
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
            args->startupInfo->dwFlags &= ~STARTF_USESTDHANDLES;
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
                mShareMemoryId.toLocal8Bit().data()
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
#else
    int BUF_SIZE=1024;
    char* pBuf=nullptr;
    int fd_shm = shm_open(mShareMemoryId.toLocal8Bit().data(),O_RDWR | O_CREAT,S_IRWXU);
    if (fd_shm==-1) {
        qDebug()<<QString("shm open failed %1:%2").arg(errno).arg(strerror(errno));
    } else {
        if (ftruncate(fd_shm,BUF_SIZE)==-1){
            qDebug()<<QString("truncate failed %1:%2").arg(errno).arg(strerror(errno));
        } else {
            pBuf = (char*)mmap(NULL,BUF_SIZE,PROT_READ | PROT_WRITE, MAP_SHARED, fd_shm,0);
            if (pBuf == MAP_FAILED) {
                qDebug()<<QString("mmap failed %1:%2").arg(errno).arg(strerror(errno));
                pBuf = nullptr;
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
        mProcess->waitForFinished(0);
    }
    bool writeChannelClosed = false;
    while (true) {
        if (mProcess->bytesToWrite()==0 && redirectInput() && !writeChannelClosed) {
            writeChannelClosed=true;
            mProcess->closeWriteChannel();
        }
        mProcess->waitForFinished(mWaitForFinishTime);
        if (mProcess->state()!=QProcess::Running) {
            break;
        }
        if (errorOccurred)
            break;
        if (mStop) {
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
        if (mStartConsole && !mPausing && pBuf) {
            if (strncmp(pBuf,"FINISHED",sizeof("FINISHED"))==0) {
#ifdef Q_OS_WIN
                if (pBuf) {
                    UnmapViewOfFile(pBuf);
                    pBuf = nullptr;
                }
                if (hSharedMemory!=INVALID_HANDLE_VALUE && hSharedMemory!=NULL) {
                    hSharedMemory = INVALID_HANDLE_VALUE;
                    CloseHandle(hSharedMemory);
                }
#else
                if (pBuf) {
                    munmap(pBuf,BUF_SIZE);
                    pBuf = nullptr;
                }
                if (fd_shm!=-1) {
                    shm_unlink(mShareMemoryId.toLocal8Bit().data());
                    fd_shm = -1;
                }
#endif
                setPausing(true);
                emit pausingForFinish();
            }
        }
    }
#ifdef Q_OS_WIN
    if (pBuf)
        UnmapViewOfFile(pBuf);
    if (hSharedMemory!=INVALID_HANDLE_VALUE && hSharedMemory!=NULL)
        CloseHandle(hSharedMemory);
#else
    if (pBuf) {
        munmap(pBuf,BUF_SIZE);
    }
    if (fd_shm!=-1) {
        shm_unlink(mShareMemoryId.toLocal8Bit().data());
    }
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
    mQuitSemaphore.release(1);
}

void ExecutableRunner::doStop()
{
    mQuitSemaphore.acquire(1);
}
