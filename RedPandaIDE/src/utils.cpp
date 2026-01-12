#include "utils.h"
#include "systemconsts.h"
#include <QDate>
#include <QDateTime>
#include <QApplication>
#include <QDesktopServices>
#include <QSysInfo>
#include <QVersionNumber>
#include <QRandomGenerator>
#include <QOperatingSystemVersion>
#include <QtXml>
#include "editor.h"
#include "compiler/executablerunner.h"
#include <QComboBox>
#include "utils/escape.h"
#include "utils/parsearg.h"
#include "settings.h"
#ifdef Q_OS_WIN
#include <QDesktopServices>
#include <windows.h>
#endif

ProcessOutput runAndGetOutput(const QString &cmd, const QString& workingDir, const QStringList& arguments,
                           const QByteArray &inputContent,
                           bool separateStderr,
                           bool inheritEnvironment,
                           const QProcessEnvironment& env)
{
    QProcess process;
    QByteArray standardOutput;
    QByteArray standardError;
    QString errorMessage;
    bool errorOccurred = false;
    if (env.isEmpty()) {
        if (inheritEnvironment) {
            process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
        } else {
            process.setProcessEnvironment(QProcessEnvironment());
        }
    } else {
        process.setProcessEnvironment(env);
    }
    if (separateStderr)
        process.setProcessChannelMode(QProcess::SeparateChannels);
    else
        process.setProcessChannelMode(QProcess::MergedChannels);
    process.setReadChannel(QProcess::StandardOutput);
    process.setWorkingDirectory(workingDir);
    process.connect(&process,&QProcess::readyReadStandardOutput,
                    [&](){
        standardOutput.append(process.readAllStandardOutput());
    });
    if (separateStderr)
        process.connect(&process, &QProcess::readyReadStandardError,
                        [&]() {
                            standardError.append(process.readAllStandardError());
                        });
    process.connect(&process, &QProcess::errorOccurred,
                    [&](){
                        errorOccurred= true;
                    });
    process.start(cmd,arguments);
    if (!inputContent.isEmpty()) {
        process.write(inputContent);
    }
    process.closeWriteChannel();
    process.waitForFinished();
    if (errorOccurred) {
        switch(process.error()) {
        case QProcess::FailedToStart:
            errorMessage += "Failed to start process!";
            break;
        case QProcess::Crashed:
            errorMessage += "Process crashed!";
            break;
        case QProcess::Timedout:
            errorMessage += "Timeout!";
            break;
        case QProcess::ReadError:
            errorMessage += "Read Error:";
            break;
        case QProcess::WriteError:
            errorMessage += "Write Error:";
            break;
        case QProcess::UnknownError:
            errorMessage += "Unknown Error:";
            break;
        }
        errorMessage += process.errorString();
    }
    return {standardOutput, standardError, errorMessage};
}

void executeFile(const QString &fileName, const QStringList &params, const QString &workingDir, const QString &tempFile)
{
    ExecutableRunner* runner=new ExecutableRunner(
                fileName,
                params,
                workingDir);
    runner->connect(runner, &QThread::finished,
                    [runner,tempFile](){
        if (!tempFile.isEmpty()) {
            QFile::remove(tempFile);
        }
        runner->deleteLater();
    });
    runner->connect(runner, &Runner::runErrorOccurred,
            [](const QString&){
        //todo
    });
    runner->setStartConsole(true);
    runner->start();
}


qulonglong stringToHex(const QString &str, bool &isOk)
{
    qulonglong value = str.toULongLong(&isOk,16);
    return value;
}

QByteArray getHTTPBody(const QByteArray& content) {
    int i= content.indexOf("\r\n\r\n");
    if (i>=0) {
        return content.mid(i+4);
    }
    return "";
}

QString getSizeString(int size)
{
    if (size < 1024) {
        return QString("%1 ").arg(size)+QObject::tr("bytes");
    } else if (size < 1024 * 1024) {
        return QString("%1 ").arg(size / 1024.0,0,'f',2)+QObject::tr("KB");
    } else if (size < 1024 * 1024 * 1024) {
        return QString("%1 ").arg(size / 1024.0 / 1024.0)+QObject::tr("MB");
    } else {
        return QString("%1 ").arg(size / 1024.0 / 1024.0 / 1024.0)+QObject::tr("GB");
    }
}

void openFileFolderInExplorer(const QString &path)
{
    QFileInfo info(path);
    if (info.isFile()){
#ifdef Q_OS_WIN
        QProcess process;
        QStringList args;
        QString filepath=QDir::toNativeSeparators(info.absoluteFilePath());
        args.append("/n,");
        args.append("/select,");
        args.append(QString("%1").arg(filepath));
        process.startDetached("explorer.exe",args);
#else
        QDesktopServices::openUrl(
                    QUrl("file:///"+
                         includeTrailingPathDelimiter(info.path()),QUrl::TolerantMode));
#endif
    } else if (info.isDir()){
        QDesktopServices::openUrl(
                    QUrl("file:///"+
                         includeTrailingPathDelimiter(path),QUrl::TolerantMode));
    }
}


QStringList getExecutableSearchPaths()
{
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    QString path = env.value("PATH");
    QStringList pathList = path.split(PATH_SEPARATOR);
#ifdef Q_OS_WINDOWS
    /* follow Windows `CreateProcessW` search semantics.
     * ref. https://learn.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw .
     */
    QStringList searchList{};
    wchar_t buffer[MAX_PATH];
    // 1. the directory from which the application loaded
    searchList.push_back(QApplication::instance()->applicationDirPath());
    // 2. the current directory for the parent process
    // here we add it because launching from GUI the current directory is relatively stable
    searchList.push_back(QDir::currentPath());
    // 3. the 32-bit Windows system directory
    if (GetSystemDirectoryW(buffer, MAX_PATH) > 0)
        searchList.push_back(QString::fromWCharArray(buffer));
    if (GetWindowsDirectoryW(buffer, MAX_PATH) > 0) {
        // 4. the 16-bit Windows system directory
        searchList.push_back(QString::fromWCharArray(buffer) + "/System");
        // 5. the Windows directory
        searchList.push_back(QString::fromWCharArray(buffer));
    }
    // 6. the directories that are listed in the PATH environment variable
    searchList.append(pathList);
    return searchList;
#else
    return pathList;
#endif
}

QStringList platformCommandForTerminalArgsPreview()
{
#ifdef Q_OS_WINDOWS
    QVersionNumber currentVersion = QVersionNumber::fromString(QSysInfo::kernelVersion());
    if (currentVersion >= QVersionNumber(6, 1))
        return {"powershell.exe", "-c", "echo hello; sleep 3"};
    else
        return {"cmd.exe", "/c", "echo hello & ping 127.0.0.1"};
#else
    return {"sh", "-c", "echo hello; sleep 3"};
#endif
}

QString byteArrayToString(const QByteArray &content, bool isUTF8)
{
    if (isUTF8)
        return QString::fromUtf8(content);
    else
        return QString::fromLocal8Bit(content);
}

QByteArray stringToByteArray(const QString &content, bool isUTF8)
{
    if (isUTF8)
        return content.toUtf8();
    else
        return content.toLocal8Bit();
}

void updateComboHistory(QStringList &historyList, const QString &newKey)
{
    int idx = historyList.indexOf(newKey);
    if (idx!=-1)
        historyList.removeAt(idx);
    if (!newKey.isEmpty())
        historyList.insert(0,newKey);
}

void setComboTextAndHistory(QComboBox *cb, const QString &newText, QStringList &historyList)
{
    int idx;
    if (!newText.isEmpty()) {
        idx = historyList.indexOf(newText);
        if (idx == -1) {
            historyList.insert(0, newText);
            idx = 0;
        }
    } else {
        idx = cb->currentIndex();
    }
    cb->clear();
    cb->addItems(historyList);
    cb->setCurrentIndex(idx);
}


