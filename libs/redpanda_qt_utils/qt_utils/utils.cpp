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
#include "utils.h"
#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QtGlobal>
#include <QDebug>
#include <QStyleFactory>
#include <QDateTime>
#include <QColor>
#include <QWindow>
#include <QScreen>
#include <QDirIterator>
#include <QTextEdit>
#include "charsetinfo.h"

#ifdef Q_OS_WIN
#include <QDirIterator>
#include <QFont>
#include <QFontMetrics>
#include <QMimeDatabase>
#include <windows.h>
#endif

#if QT_VERSION_MAJOR >= 6
# include <QStringConverter>
#else
# include <QTextCodec>
#endif

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

QString guessTextEncoding(const QByteArray& text){
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



bool isTextAllAscii(const QByteArray& text) {
    for (QChar c:text) {
        if (c.unicode()>127) {
            return false;
        }
    }
    return true;
}

bool isTextAllAscii(const QString& text) {
    for (QChar c:text) {
        if (c.unicode()>127) {
            return false;
        }
    }
    return true;
}

bool isNonPrintableAsciiChar(char ch)
{
    return (ch<=32) && (ch>=0);
}

QStringList textToLines(const QString &text)
{
    QTextStream stream(&((QString&)text),QIODevice::ReadOnly);
    return readStreamToLines(&stream);
}


void textToLines(const QString &text, LineProcessFunc lineFunc)
{
    QTextStream stream(&((QString&)text),QIODevice::ReadOnly);
    readStreamToLines(&stream,lineFunc);
}

QString linesToText(const QStringList &lines, const QString& lineBreak)
{
    return lines.join(lineBreak);
}

QList<QByteArray> splitByteArrayToLines(const QByteArray &content)
{
    QList<QByteArray> lines;
    const char* p =content.constData();
    const char* end = p+content.length();
    const char* lineStart = p;
    QByteArray line;
    while (p<=end) {
        char ch=*p;
        switch(ch) {
        case '\r':
            line = QByteArray(lineStart, p-lineStart);
            lines.append(line);
            p++;
            if (*p=='\n')
                p++;
            lineStart = p;
            break;
        case '\n':
            line = QByteArray(lineStart, p-lineStart);
            lines.append(line);
            p++;
            lineStart = p;
            break;
        default:
            p++;
        }
    }
    if (lineStart>end) {
        lines.append("");
    } else {
        line = QByteArray(lineStart, end-lineStart+1);
        lines.append(line);
    }
    return lines;
}

QString trimRight(const QString &s)
{
    if (s.isEmpty())
        return s;
    int i = s.length()-1;
//   while ((i>=0) && ((s[i] == '\r') || (s[i]=='\n') || (s[i] == '\t') || (s[i]==' ')))  {
    while ((i>=0) && ((s[i] == '\t') || (s[i]==' '))) {
        i--;
    };
    if (i>=0) {
        return s.left(i+1);
    } else {
        return QString();
    }
}

QString trimLeft(const QString &s)
{
    if (s.isEmpty())
        return s;
    int i=0;
//    while ((i<s.length()) && ((s[i] == '\r') || (s[i]=='\n') || (s[i] == '\t') || (s[i]==' ')))  {
//        i++;
//    };
    while ((i<s.length()) && ((s[i] == '\t') || (s[i]==' ')))  {
        i++;
    };
    if (i<s.length()) {
        return s.mid(i);
    } else {
        return QString();
    }
}

int countLeadingWhitespaceChars(const QString &line)
{
    int n=0;
    while (n<line.length()) {
        if (line[n].unicode()>32)
            break;
        n++;
    }
    return n;
}

bool stringIsBlank(const QString &s)
{
    for (QChar ch:s) {
        if (ch != ' ' && ch != '\t')
            return false;
    }
    return true;
}

QByteArray toByteArray(const QString &s)
{
    //return s.toLocal8Bit();
    return s.toUtf8();
}

QString fromByteArray(const QByteArray &s)
{
#ifdef Q_OS_WIN
    TextDecoder decoder = TextDecoder::decoderForUtf8();
    auto [ok, result] = decoder.decode(s);
    if (ok)
        return result;
    else
        return QString::fromLocal8Bit(s);
#else
    return QString::fromUtf8(s);
#endif
}

QStringList readStreamToLines(QTextStream *stream)
{
    QStringList list;
    QString s;
    while (stream->readLineInto(&s)) {
        list.append(s);
    }
    return list;
}

void readStreamToLines(QTextStream *stream,
                              LineProcessFunc lineFunc)
{
    QString s;
    while (stream->readLineInto(&s)) {
        lineFunc(s);
    }
}

static QStringList tryLoadFileByEncoding(QByteArray encodingName, QFile& file, bool* isOk) {
    QStringList result;
    *isOk = false;
    TextDecoder decoder(encodingName);
    if (!decoder.isValid())
        return result;
    file.reset();
    while (true) {
        if (file.atEnd()){
            break;
        }
        QByteArray line = file.readLine();
        if (line.endsWith("\r\n")) {
            line.remove(line.length()-2,2);
        } else if (line.endsWith("\r")) {
            line.remove(line.length()-1,1);
        } else if (line.endsWith("\n")){
            line.remove(line.length()-1,1);
        }
        auto [ok, newLine] = decoder.decode(line);
        if (!ok) {
            return QStringList();
        }
        result.append(newLine);
    }
    *isOk=true;
    return result;
}

QStringList readFileToLines(const QString &fileName)
{
    QFile file(fileName);
    if (file.size()<=0)
        return QStringList();
    QStringList result;
    if (file.open(QFile::ReadOnly)) {
        bool ok;
        result = tryLoadFileByEncoding("UTF-8",file,&ok);
        if (ok) {
            return result;
        }

        QByteArray realEncoding = pCharsetInfoManager->getDefaultSystemEncoding();
        result = tryLoadFileByEncoding(realEncoding,file,&ok);
        if (ok) {
            return result;
        }
        QList<PCharsetInfo> charsets = pCharsetInfoManager->findCharsetByLocale(pCharsetInfoManager->localeName());
        if (!charsets.isEmpty()) {
            QSet<QString> encodingSet;
            for (int i=0;i<charsets.size();i++) {
                encodingSet.insert(charsets[i]->name);
            }
            encodingSet.remove(realEncoding);
            foreach (const QString& encodingName,encodingSet) {
                if (encodingName == ENCODING_UTF8)
                    continue;
                result = tryLoadFileByEncoding("UTF-8",file,&ok);
                if (ok) {
                    return result;
                }
            }
        }
    }
    return result;
}

QByteArray readFileToByteArray(const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        return file.readAll();
    }
    return QByteArray();
}

bool stringToFile(const QString &str, const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    QTextStream stream(&file);
    stream<<str;
    return true;
}


bool stringsToFile(const QStringList &list, const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    QTextStream stream(&file);
    for (const QString& s:list) {
        stream<<s<<Qt::endl;
    }
    return true;
}

bool fileExists(const QString &file)
{
    if (file.isEmpty())
        return false;
    return QFile(file).exists();
}

bool fileExists(const QString &dir, const QString &fileName)
{
    if (dir.isEmpty() || fileName.isEmpty())
        return false;
    QDir dirInfo(dir);
    return dirInfo.exists(fileName);
}

bool directoryExists(const QString &file)
{
    if (file.isEmpty())
        return false;
   QFileInfo dir(file);
   return dir.exists() && dir.isDir();
}

bool removeFile(const QString &filename)
{
    QFile file(filename);
    return file.remove();
}

bool copyFile(const QString &fromPath, const QString &toPath, bool overwrite)
{
    QFile  fromFile(fromPath);
    QFile toFile(toPath);
    if (!fromFile.exists())
        return false;
    if (toFile.exists()) {
        if (!overwrite)
            return false;
        if (!toFile.remove())
            return false;
    }

    if (!fromFile.open(QFile::ReadOnly))
        return false;
    if (!toFile.open(QFile::WriteOnly | QFile::Truncate))
        return false;

    constexpr int bufferSize=64*1024;
    char buffer[bufferSize];

    while (!fromFile.atEnd()) {
        int readed = fromFile.read(buffer,bufferSize);
        toFile.write(buffer,readed);
    }
    toFile.close();
    fromFile.close();
    return true;
}

void copyFolder(const QString &fromDir, const QString &toDir)
{
    QDirIterator it(fromDir);
    QDir dir(fromDir);
    QDir targetDir(toDir);
    const int absSourcePathLength = dir.absolutePath().length();


    if (targetDir.exists())
        return;
    targetDir.mkpath(targetDir.absolutePath());

    while (it.hasNext()){
        it.next();
        const auto fileInfo = it.fileInfo();
        if(!fileInfo.isHidden() && !fileInfo.fileName().startsWith('.')) { //filters dot and dotdot
            const QString subPathStructure = fileInfo.absoluteFilePath().mid(absSourcePathLength);
            const QString constructedAbsolutePath = targetDir.absolutePath() + subPathStructure;
            if(fileInfo.isDir()){
                //Create directory in target folder
                dir.mkpath(constructedAbsolutePath);
                copyFolder(fileInfo.absoluteFilePath(), constructedAbsolutePath);
            } else if(fileInfo.isFile()) {
                //Copy File to target directory

                //Remove file at target location, if it exists, or QFile::copy will fail
                QFile::remove(constructedAbsolutePath);
                QFile::copy(fileInfo.absoluteFilePath(), constructedAbsolutePath);
                QFile newFile(constructedAbsolutePath);
                QFile::Permissions permissions = newFile.permissions();
                permissions |= (QFile::Permission::WriteOwner
                                | QFile::Permission::WriteUser
                                | QFile::Permission::WriteGroup
                                | QFile::Permission::WriteOther);
                newFile.setPermissions(permissions);
            }
        }
    }
}

QString includeTrailingPathDelimiter(const QString &path)
{
    if (path.endsWith(QDir::separator()) || path.endsWith(QDir::separator())) {
        return path;
    } else {
        return path + QDir::separator();
    }
}

QString excludeTrailingPathDelimiter(const QString &path)
{
    int pos = path.length()-1;
    while (pos>=0 && (path[pos]=='/' || path[pos]==QDir::separator()))
        pos--;
    return path.mid(0,pos+1);
}

QString changeFileExt(const QString& filename, QString ext)
{
    QFileInfo fileInfo(filename);
    QString suffix = fileInfo.suffix();
    QString name  = fileInfo.fileName();
    QString path;
    if (!ext.isEmpty() && !ext.startsWith(".")) {
        ext = "."+ext;
    }
    if (fileInfo.path() != ".") {
        path = includeTrailingPathDelimiter(fileInfo.path());
    }
    if (suffix.isEmpty()) {
        return path+name+ext;
    } else {
        return path+fileInfo.completeBaseName()+ext;
    }
}

QString extractRelativePath(const QString &base, const QString &dest)
{
    if (dest.isEmpty())
        return QString();

    QFileInfo baseInfo(base);
    QDir baseDir;
    if (baseInfo.isDir()) {
        baseDir = QDir(baseInfo.absoluteFilePath());
    } else {
        baseDir = baseInfo.absoluteDir();
    }
    return baseDir.relativeFilePath(dest);
}

QString localizePath(const QString &path)
{
    QString result = path;
    result.replace("/",QDir::separator());
    return result;
}

QString extractFileName(const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    return fileInfo.fileName();
}

QString extractFileDir(const QString &fileName)
{
    return extractFilePath(fileName);
}

QString extractFilePath(const QString &filePath)
{
    QFileInfo info(filePath);
    return info.path();
}

QString extractAbsoluteFilePath(const QString &filePath)
{
    QFileInfo info(filePath);
    return info.absoluteFilePath();
}

bool isReadOnly(const QString &filename)
{
    return QFile(filename).isWritable();
}

int compareFileModifiedTime(const QString &filename1, const QString &filename2)
{
    QFileInfo fileInfo1(filename1);
    QFileInfo fileInfo2(filename2);
    qint64 time1=fileInfo1.lastModified().toMSecsSinceEpoch();
    qint64 time2=fileInfo2.lastModified().toMSecsSinceEpoch();
    if (time1 > time2)
        return 1;
    if (time1 < time2)
        return -1;
    return 0;
}


void inflateRect(QRectF &rect, float delta)
{
    inflateRect(rect,delta,delta);
}

void inflateRect(QRectF &rect, float dx, float dy)
{
    rect.setLeft(rect.left()-dx);
    rect.setRight(rect.right()+dx);
    rect.setTop(rect.top()-dy);
    rect.setBottom(rect.bottom()+dy);
}


static int defaultScreenDPI = -1;

int screenDPI()
{
    if (defaultScreenDPI<1) {
        defaultScreenDPI = qApp->primaryScreen()->logicalDotsPerInch();
    }
    return defaultScreenDPI;
}


void setScreenDPI(int dpi)
{
    defaultScreenDPI = dpi;
}

float pointToPixel(float point, float dpi)
{
     return point * dpi / 72;
}

float pointToPixel(float point)
{
    return pointToPixel(point,screenDPI());
}

float pixelToPoint(float pixel)
{
    return pixel * 72 / screenDPI();
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

bool isInFolder(const QString &folderpath, const QString &filepath)
{
    QDir folder(folderpath);
    QFileInfo fileInfo(filepath);
    return fileInfo.absoluteFilePath().startsWith(includeTrailingPathDelimiter(folder.absolutePath()));
}

void createFile(const QString &fileName)
{
    stringToFile("",fileName);
}

QString cleanPath(const QString &dirPath)
{
    return QDir::cleanPath(dirPath);
}

QString generateAbsolutePath(const QString &dirPath, const QString &relativePath)
{
    if (relativePath.isEmpty())
        return QString();
    return QDir::cleanPath(QDir(dirPath).absoluteFilePath(relativePath));
}

QString escapeSpacesInString(const QString &str)
{
    QString result=str;
    return result.replace(' ',"%20");
}

QStringList extractRelativePaths(const QString &base, const QStringList &destList)
{
    QStringList list;
    foreach(const QString& dest,destList) {
        list.append(extractRelativePath(base,dest));
    }
    return list;
}

QStringList absolutePaths(const QString &dirPath, const QStringList &relativePaths)
{
    QStringList list;
    foreach(const QString& path,relativePaths) {
        list.append(generateAbsolutePath(dirPath,path));
    }
    return list;
}

bool isBinaryContent(const QByteArray &text)
{
    for (char c:text) {
        if (c==0) {
            return true;
        }
    }
    return false;
}

void clearQPlainTextEditFormat(QTextEdit *editor)
{
    QTextCursor cursor = editor->textCursor();
    cursor.select(QTextCursor::Document);
    cursor.setCharFormat(QTextCharFormat());
    cursor.clearSelection();
}

int compareFileModifiedTime(const QString &filename, qint64 timestamp)
{
    QFileInfo fileInfo1(filename);
    qint64 time=fileInfo1.lastModified().toMSecsSinceEpoch();
    if (time > timestamp)
        return 1;
    if (time < timestamp)
        return -1;
    return 0;
}

QString replacePrefix(const QString &oldString, const QString &prefix, const QString &newPrefix)
{
    QString result = oldString;
    if (oldString.startsWith(prefix)) {
        result = newPrefix+oldString.mid(prefix.length());
    }
    return result;
}

const QChar *getNullTerminatedStringData(const QString &str)
{
    const QChar* result = str.constData();
    if (result[str.size()]!=QChar(0)) {
        result = str.data();
    }
    return result;
}

TextEncoder::TextEncoder(const char *name)
{
#if QT_VERSION_MAJOR >= 6
    mEncoder = QStringEncoder(name, QStringConverter::Flag::Stateless);
#else
    mCodec = QTextCodec::codecForName(name);
#endif
}

TextEncoder::TextEncoder(const QByteArray &name) :
    TextEncoder(name.data())
{
}

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
TextEncoder::TextEncoder(QStringEncoder &&encoder)
{
    mEncoder = std::move(encoder);
}
#endif

bool TextEncoder::isValid() const
{
#if QT_VERSION_MAJOR >= 6
    return mEncoder.isValid();
#else
    return mCodec != nullptr;
#endif
}

QByteArray TextEncoder::name() const
{
#if QT_VERSION_MAJOR >= 6
    return mEncoder.name();
#else
    return mCodec->name();
#endif
}

std::pair<bool, QByteArray> TextEncoder::encode(const QString &text)
{
    if (!isValid())
        return {false, QByteArray()};
    QByteArray result;
#if QT_VERSION_MAJOR >= 6
    result = mEncoder(text);
    if (mEncoder.hasError()) {
        mEncoder.resetState();
        return {false, QByteArray()};
    }
#else
    QTextCodec::ConverterState state;
    result = mCodec->fromUnicode(text.constData(), text.length(), &state);
    if (state.invalidChars > 0)
        return {false, QByteArray()};
#endif
    return {true, result};
}

QByteArray TextEncoder::encodeUnchecked(const QString &text)
{
    if (!isValid())
        return QByteArray();
#if QT_VERSION_MAJOR >= 6
    QByteArray result = mEncoder(text);
    if (mEncoder.hasError())
        mEncoder.resetState();
    return result;
#else
    return mCodec->fromUnicode(text);
#endif
}

TextEncoder TextEncoder::encoderForUtf8()
{
#if QT_VERSION_MAJOR >= 6
    return QStringEncoder(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
#else
    return TextEncoder(ENCODING_UTF8);
#endif
}

TextEncoder TextEncoder::encoderForUtf16()
{
#if QT_VERSION_MAJOR >= 6
    return QStringEncoder(QStringConverter::Utf16, QStringConverter::Flag::Stateless);
#else
    return TextEncoder(ENCODING_UTF16);
#endif
}

TextEncoder TextEncoder::encoderForUtf32()
{
#if QT_VERSION_MAJOR >= 6
    return QStringEncoder(QStringConverter::Utf32, QStringConverter::Flag::Stateless);
#else
    return TextEncoder(ENCODING_UTF32);
#endif
}

TextEncoder TextEncoder::encoderForSystem()
{
#ifdef Q_OS_WIN
# if QT_VERSION_MAJOR >= 6
    return QStringEncoder(QStringConverter::System, QStringConverter::Flag::Stateless);
# else
    return TextEncoder(ENCODING_SYSTEM_DEFAULT);
# endif
#else
    return encoderForUtf8();
#endif
}

TextDecoder::TextDecoder(const char *name)
{
#if QT_VERSION_MAJOR >= 6
    mDecoder = QStringDecoder(name, QStringConverter::Flag::Stateless);
#else
    mCodec = QTextCodec::codecForName(name);
#endif
}

TextDecoder::TextDecoder(const QByteArray &name) :
    TextDecoder(name.data())
{
}

#if QT_VERSION_MAJOR >= 6
TextDecoder::TextDecoder(QStringDecoder &&decoder)
{
    mDecoder = std::move(decoder);
}
#endif

bool TextDecoder::isValid() const
{
#if QT_VERSION_MAJOR >= 6
    return mDecoder.isValid();
#else
    return mCodec != nullptr;
#endif
}

QByteArray TextDecoder::name() const
{
#if QT_VERSION_MAJOR >= 6
    return mDecoder.name();
#else
    return mCodec->name();
#endif
}

std::pair<bool, QString> TextDecoder::decode(const QByteArray &text)
{
    if (!isValid())
        return {false, QString()};
    QString result;
#if QT_VERSION_MAJOR >= 6
    result = mDecoder(text);
    if (mDecoder.hasError()) {
        mDecoder.resetState();
        return {false, QString()};
    }
#else
    QTextCodec::ConverterState state;
    result = mCodec->toUnicode(text.constData(), text.length(), &state);
    if (state.invalidChars > 0)
        return {false, QString()};
#endif
    return {true, result};
}

QString TextDecoder::decodeUnchecked(const QByteArray &text)
{
    if (!isValid())
        return QString();
#if QT_VERSION_MAJOR >= 6
    QString result = mDecoder(text);
    if (mDecoder.hasError())
        mDecoder.resetState();
    return result;
#else
    return mCodec->toUnicode(text);
#endif
}

TextDecoder TextDecoder::decoderForUtf8()
{
#if QT_VERSION_MAJOR >= 6
    return QStringDecoder(QStringConverter::Utf8, QStringConverter::Flag::Stateless);
#else
    return TextDecoder(ENCODING_UTF8);
#endif
}

TextDecoder TextDecoder::decoderForUtf16()
{
#if QT_VERSION_MAJOR >= 6
    return QStringDecoder(QStringConverter::Utf16, QStringConverter::Flag::Stateless);
#else
    return TextDecoder(ENCODING_UTF16);
#endif
}

TextDecoder TextDecoder::decoderForUtf32()
{
#if QT_VERSION_MAJOR >= 6
    return QStringDecoder(QStringConverter::Utf32, QStringConverter::Flag::Stateless);
#else
    return TextDecoder(ENCODING_UTF32);
#endif
}

TextDecoder TextDecoder::decoderForSystem()
{
#ifdef Q_OS_WIN
# if QT_VERSION_MAJOR >= 6
    return QStringDecoder(QStringConverter::System, QStringConverter::Flag::Stateless);
# else
    return TextDecoder(ENCODING_SYSTEM_DEFAULT);
# endif
#else
    return decoderForUtf8();
#endif
}

const QStringList &availableEncodings() {
    static bool initialized = false;
    static QStringList encodings;

    if (initialized)
        return encodings;

#if QT_VERSION_MAJOR >= 6
    for (const QString &name : QStringConverter::availableCodecs()) {
        QString lname = name.toLower();
        if (lname.startsWith("cp"))
            continue;
        if (lname == "locale" || lname == "utf-8")
            continue;
        encodings.append(name);
    }
#else
    QSet<QByteArray> codecAlias = {"system", "utf-8"};
    for (const QByteArray &name : QTextCodec::availableCodecs()) {
        QByteArray lname = name.toLower();
        if (lname.startsWith("cp"))
            continue;
        if (codecAlias.contains(lname))
            continue;
        encodings.append(name);
        codecAlias.insert(lname);
        QTextCodec *codec = QTextCodec::codecForName(name);
        if (codec != nullptr) {
            for (const QByteArray &alias : codec->aliases())
                codecAlias.insert(alias.toLower());
        }
    }
#endif
    std::sort(encodings.begin(), encodings.end());
    initialized = true;
    return encodings;
}

bool isEncodingAvailable(const QByteArray &encoding)
{
    TextEncoder encoder(encoding);
    return encoder.isValid();
}
