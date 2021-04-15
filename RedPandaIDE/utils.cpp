#include "utils.h"
#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QSettings>
#include <QString>
#include <QTextCodec>

const QByteArray GuessTextEncoding(const QByteArray& text){
    bool allAscii;
    int ii;
    int size;
    const QByteArray& s=text;
    size = s.length();
    if ( (size >= 3) && ((unsigned char)s[0]==0xEF) && ((unsigned char)s[1]==0xBB) && ((unsigned char)s[2]==0xBF)) {
        return ENCODING_UTF8_BOM;
    }
    allAscii = true;
    ii = 0;
    while (ii < size) {
        unsigned char ch = s[ii];
        if (ch < 0x80 ) {
            ii++; // is an ascii char
        } else if (ch < 0xC0) { // value between 0x80 and 0xC0 is an invalid UTF-8 char
            return ENCODING_SYSTEM_DEFAULT;
        } else if (ch < 0xE0) { // should be an 2-byte UTF-8 char
            if (ii>=size-1) {
                return ENCODING_SYSTEM_DEFAULT;
            }
            unsigned char ch2=s[ii+1];
            if ((ch2 & 0xC0) !=0x80)  {
                return ENCODING_SYSTEM_DEFAULT;
            }
            allAscii = false;
            ii+=2;
        } else if (ch < 0xF0) { // should be an 3-byte UTF-8 char
            if (ii>=size-2) {
                return ENCODING_SYSTEM_DEFAULT;
            }
            unsigned char ch2=s[ii+1];
            unsigned char ch3=s[ii+2];
            if (((ch2 & 0xC0)!=0x80) ||  ((ch3 & 0xC0)!=0x80)) {
                return ENCODING_SYSTEM_DEFAULT;
            }
            allAscii = false;
            ii+=3;
        } else { // invalid UTF-8 char
            return ENCODING_SYSTEM_DEFAULT;
        }
    }
    if (allAscii)
        return ENCODING_ASCII;
    return ENCODING_UTF8;
}

bool isTextAllAscii(const QString& text) {
    for (QChar c:text) {
        if (c.unicode()>127) {
            return false;
        }
    }
    return true;
}


static bool gIsGreenEdition = false;
static bool gIsGreenEditionInited = false;
bool isGreenEdition()
{
    if (!gIsGreenEditionInited) {
        QSettings settings("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\RedPanda-C++",
                           QSettings::NativeFormat);
        QString regPath = QFileInfo(settings.value("UninstallString").toString()).absolutePath();

        QString appPath = QApplication::instance()->applicationDirPath();
        gIsGreenEdition = (regPath != appPath);
        gIsGreenEditionInited = true;
    }
    return gIsGreenEdition;
}

QByteArray runAndGetOutput(const QString &cmd, const QString& workingDir, const QStringList& arguments, bool inheritEnvironment)
{
    QProcess process;
    QByteArray result;
    if (inheritEnvironment) {
        process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
    } else {
        process.setProcessEnvironment(QProcessEnvironment());
    }
    process.setWorkingDirectory(workingDir);
    process.start(cmd,arguments,QIODevice::ReadOnly);
    process.closeWriteChannel();
    process.connect(&process,&QProcess::readyReadStandardError,
                    [&](){
        result.append(process.readAllStandardError());
    });
    process.connect(&process,&QProcess::readyReadStandardOutput,
                    [&](){
        result.append(process.readAllStandardOutput());
    });
    process.waitForFinished();
    return result;
}

bool isNonPrintableAsciiChar(char ch)
{
    return (ch>32) or (ch<0);
}

bool fileExists(const QString &file)
{
    return QFileInfo(file).exists();
}

bool fileExists(const QString &dir, const QString &fileName)
{
    QDir dirInfo(dir);
    return dirInfo.exists(fileName);
}

bool directoryExists(const QString &file)
{
   QFileInfo dir(file);
   return dir.exists() && dir.isDir();
}
