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
#include <windows.h>

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
    process.waitForFinished();
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

bool programHasConsole(const QString &filename)
{
    bool result = false;
    HANDLE handle = CreateFile(filename.toStdWString().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle != INVALID_HANDLE_VALUE) {
        IMAGE_DOS_HEADER dos_header;
        DWORD signature;
        DWORD bytesread;
        IMAGE_FILE_HEADER pe_header;
        IMAGE_OPTIONAL_HEADER opt_header;

        ReadFile(handle, &dos_header, sizeof(dos_header), &bytesread, NULL);
        SetFilePointer(handle, dos_header.e_lfanew, NULL, 0);
        ReadFile(handle, &signature, sizeof(signature), &bytesread, NULL);
        ReadFile(handle, &pe_header, sizeof(pe_header), &bytesread, NULL);
        ReadFile(handle, &opt_header, sizeof(opt_header), &bytesread, NULL);

        result = (opt_header.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_CUI);
    }
    CloseHandle(handle);
    return result;
}

QString toLocalPath(const QString &filename)
{
    QString newPath {filename};
    newPath.replace("/",QDir::separator());
    return newPath;
}

QStringList TextToLines(const QString &text)
{
    QTextStream stream(&((QString&)text),QIODevice::ReadOnly);
    return ReadStreamToLines(&stream);
}

QStringList ReadFileToLines(const QString& fileName, QTextCodec* codec)
{
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);
        stream.setCodec(codec);
        stream.setAutoDetectUnicode(false);
        return ReadStreamToLines(&stream);
    }
    return QStringList();
}

QStringList ReadStreamToLines(QTextStream *stream)
{
    QStringList list;
    QString s;
    while (stream->readLineInto(&s)) {
        list.append(s);
    }
    return list;
}

void ReadStreamToLines(QTextStream *stream,
                              LineProcessFunc lineFunc)
{
    QString s;
    while (stream->readLineInto(&s)) {
        lineFunc(s);
    }
}

void TextToLines(const QString &text, LineProcessFunc lineFunc)
{
    QTextStream stream(&((QString&)text),QIODevice::ReadOnly);
    ReadStreamToLines(&stream,lineFunc);
}

void ReadFileToLines(const QString &fileName, QTextCodec *codec, LineProcessFunc lineFunc)
{
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);
        stream.setCodec(codec);
        stream.setAutoDetectUnicode(false);
        ReadStreamToLines(&stream, lineFunc);
    }
}

BaseError::BaseError(const QString &reason):
mReason(reason)
{

}

QString BaseError::reason() const
{
    return mReason;
}

IndexOutOfRange::IndexOutOfRange(int Index):
BaseError(QObject::tr("Index %1 out of range").arg(Index))
{

}

FileError::FileError(const QString &reason): BaseError(reason)
{

}

void decodeKey(const int combinedKey, int &key, Qt::KeyboardModifiers &modifiers)
{
    modifiers = Qt::NoModifier;
    if (combinedKey & Qt::ShiftModifier) {
        modifiers|=Qt::ShiftModifier;
    }
    if (combinedKey & Qt::ControlModifier) {
        modifiers|=Qt::ControlModifier;
    }
    if (combinedKey & Qt::AltModifier) {
        modifiers|=Qt::AltModifier;
    }
    if (combinedKey & Qt::MetaModifier) {
        modifiers|=Qt::MetaModifier;
    }
    if (combinedKey & Qt::KeypadModifier) {
        modifiers|= Qt::KeypadModifier;
    }
    key = combinedKey & ~(Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier | Qt::KeypadModifier);
}

void inflateRect(QRect &rect, int delta)
{
    inflateRect(rect,delta,delta);
}

void inflateRect(QRect &rect, int dx, int dy)
{
    rect.setLeft(rect.left()-dx);
    rect.setRight(rect.right()+dx);
    rect.setTop(rect.top()-dy);
    rect.setBottom(rect.bottom()+dy);
}

QString TrimRight(const QString &s)
{
    if (s.isEmpty())
        return s;
    int i = s.length()-1;
    while ((i>=0) && ((s[i] == '\r') || (s[i]=='\n')))  {
        i--;
    };
    if (i>=0) {
        return s.left(i+1);
    } else {
        return QString();
    }
}
