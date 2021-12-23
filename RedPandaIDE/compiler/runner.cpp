#include "runner.h"

Runner::Runner(const QString &filename, const QString &arguments, const QString &workDir
               ,QObject *parent) : QThread(parent),
    mPausing(false),
    mStop(false),
    mFilename(filename),
    mArguments(arguments),
    mWorkDir(workDir)
{

}

void Runner::stop()
{
    mStop = true;
    doStop();
}

void Runner::doStop()
{

}

bool Runner::pausing() const
{
    return mPausing;
}

void Runner::setPausing(bool newCanFinish)
{
    mPausing = newCanFinish;
}

