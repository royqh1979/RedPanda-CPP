#include "utils.h"
#include "systemconsts.h"
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
#include <QtGlobal>
#include <QDebug>

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
    process.connect(&process,&QProcess::readyReadStandardError,
                    [&](){
        result.append(process.readAllStandardError());
    });
    process.connect(&process,&QProcess::readyReadStandardOutput,
                    [&](){
        result.append(process.readAllStandardOutput());
    });
    process.start(cmd,arguments);
    process.closeWriteChannel();

    process.waitForFinished(-1);
    return result;
}

bool isNonPrintableAsciiChar(char ch)
{
    return (ch<=32) and (ch>=0);
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

QString includeTrailingPathDelimiter(const QString &path)
{
    if (path.endsWith('/') || path.endsWith(QDir::separator())) {
        return path;
    } else {
        return path + "/";
    }
}

QString excludeTrailingPathDelimiter(const QString &path)
{
    int pos = path.length()-1;
    while (pos>=0 && (path[pos]=='/' || path[pos]==QDir::separator()))
        pos--;
    return path.mid(0,pos+1);
}

FileType getFileType(const QString &filename)
{
    if (filename.endsWith(".c",PATH_SENSITIVITY)) {
        return FileType::CSource;
    }
    if (filename.endsWith(".cpp",PATH_SENSITIVITY)) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".cc",PATH_SENSITIVITY)) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".cxx",PATH_SENSITIVITY)) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".c++",PATH_SENSITIVITY)) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".h",PATH_SENSITIVITY)) {
        return FileType::CHeader;
    }
    if (filename.endsWith(".hpp",PATH_SENSITIVITY)) {
        return FileType::CppHeader;
    }
    if (filename.endsWith(".hh",PATH_SENSITIVITY)) {
        return FileType::CppHeader;
    }
    if (filename.endsWith(".hxx",PATH_SENSITIVITY)) {
        return FileType::CppHeader;
    }
    if (filename.endsWith(".inl",PATH_SENSITIVITY)) {
        return FileType::CppHeader;
    }
    if (filename.endsWith(".res",PATH_SENSITIVITY)) {
        return FileType::WindowsResourceSource;
    }
    return FileType::Other;
}


QString getCompiledExecutableName(const QString filename)
{
    QFileInfo info(filename);
    QString baseName = includeTrailingPathDelimiter(info.absolutePath())+info.baseName();
    return baseName + EXECUTABE_EXT;
}

void splitStringArguments(const QString &arguments, QStringList &argumentList)
{
    QString word;
    bool inQuota;
    inQuota = false;
    for (QChar ch:arguments) {
        if (ch == '"') {
            inQuota = !inQuota;
        } else if (ch == '\n' || ch == ' ' || ch == '\t' || ch == '\r') {
            if (!inQuota) {
                word = word.trimmed();
                if (!word.isEmpty()) {
                    argumentList.append(word);
                }
                word = "";
            } else {
                word.append(ch);
            }
        } else {
            word.append(ch);
        }
    }
    word = word.trimmed();
    if (!word.isEmpty()) {
        argumentList.append(word);
    }
}
