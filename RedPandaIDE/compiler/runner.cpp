#include "runner.h"

Runner::Runner(const QString &filename, const QString &arguments, const QString &workDir
               ,QObject *parent) : QThread(parent),
    mStop(false),
    mFilename(filename),
    mArguments(arguments),
    mWorkDir(workDir)
{

}

void Runner::stop()
{
    mStop = true;
}

