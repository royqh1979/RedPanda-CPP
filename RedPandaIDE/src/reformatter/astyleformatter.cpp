#include "astyleformatter.h"
#include "../utils/escape.h"
#include "../utils.h"
#include <QByteArray>

AStyleReformatter::AStyleReformatter(const QString& astylePath, const QStringList& args, LoggerFunc newLoggerFunc, QObject *parent):
    BaseReformatter{parent},
    mAstylePath{astylePath},
    mArgs{args},
    mLoggerFunc{newLoggerFunc}
{

}

QString AStyleReformatter::refomat(const QString &content, QString &errorMessage, bool &isOk)
{
    isOk = false;
    if (!fileExists(mAstylePath)) {
        errorMessage = tr("Can't find astyle in \"%1\".").arg(mAstylePath);
        return QString();
    }
    QByteArray byteContent = content.toUtf8();
    QString command = escapeCommandForPlatformShell(extractFileName(mAstylePath), mArgs);
    if (mLoggerFunc) {
        mLoggerFunc(tr("Reformatting content using astyle..."));
        mLoggerFunc("------------------");
        mLoggerFunc(tr("- Astyle: %1").arg(mAstylePath));
        mLoggerFunc(tr("- Command: %1").arg(command));
    }
    auto [newContent, astyleError, processError] =
        runAndGetOutput(mAstylePath, extractFileDir(mAstylePath), mArgs, byteContent, true);
    if (!astyleError.isEmpty()) {
#ifdef Q_OS_WIN
        errorMessage = QString::fromLocal8Bit(astyleError);
#else
        errorMessage = QString::fromUtf8(astyleError);
#endif
        if (mLoggerFunc)
            mLoggerFunc(errorMessage);
        return QString();
    }
    if (!processError.isEmpty()) {
        if (mLoggerFunc)
            mLoggerFunc(processError);
        return QString();
    }
    isOk = true;
    return QString::fromUtf8(newContent);
}
