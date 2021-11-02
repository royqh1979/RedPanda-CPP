#include "ojproblemcasesrunner.h"
#include "../utils.h"
#include "../settings.h"
#include "../systemconsts.h"
#include "../widgets/ojproblemsetmodel.h"
#include <QProcess>

OJProblemCasesRunner::OJProblemCasesRunner(const QString& filename, const QString& arguments, const QString& workDir,
                                           const QVector<POJProblemCase>& problemCases, QObject *parent):
    Runner(filename,arguments,workDir,parent)
{
    mProblemCases = problemCases;
}

OJProblemCasesRunner::OJProblemCasesRunner(const QString& filename, const QString& arguments, const QString& workDir,
                                           POJProblemCase problemCase, QObject *parent):
    Runner(filename,arguments,workDir,parent)
{
    mProblemCases.append(problemCase);
}

void OJProblemCasesRunner::runCase(int index,POJProblemCase problemCase)
{
    emit caseStarted(problemCase->getId(),index, mProblemCases.count());
    auto action = finally([this,&index, &problemCase]{
        emit caseFinished(problemCase->getId(), index, mProblemCases.count());
    });
    QProcess process;
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
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.connect(&process, &QProcess::errorOccurred,
                    [&](){
                        errorOccurred= true;
                    });
    problemCase->output.clear();
    process.start();
    process.waitForStarted(5000);
    if (process.state()==QProcess::Running) {
        process.write(problemCase->input.toUtf8());
        process.closeWriteChannel();
    }
    QByteArray readed;
    while (true) {
        process.waitForFinished(1000);
        readed += process.readAll();
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
    readed += process.readAll();
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
    problemCase->output = QString::fromUtf8(readed);
}

void OJProblemCasesRunner::run()
{
    emit started();
    auto action = finally([this]{
        emit terminated();
    });
    for (int i=0;i<mProblemCases.size();i++) {
        if (mStop)
            break;
        POJProblemCase problemCase =mProblemCases[i];
        runCase(i,problemCase);
    }
}


