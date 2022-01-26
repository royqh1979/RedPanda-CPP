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
#include "systemconsts.h"
#include <QApplication>
#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QProcessEnvironment>
#include <QString>
#include <QTextCodec>
#include <QtGlobal>
#include <QDebug>
#include <QStyleFactory>
#include <QDateTime>
#include <QColor>
#include <QWindow>
#include <QScreen>
#include "parser/cppparser.h"
#include "settings.h"
#include "mainwindow.h"
#include "editorlist.h"
#include "editor.h"
#include "project.h"
#include "compiler/executablerunner.h"
#ifdef Q_OS_WIN
#include <windows.h>
#endif

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

bool isTextAllAscii(const QByteArray& text) {
    for (char c:text) {
        if (c<0) {
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


static bool gIsGreenEdition = true;
static bool gIsGreenEditionInited = false;
bool isGreenEdition()
{
#ifdef Q_OS_WIN
    if (!gIsGreenEditionInited) {
        QString keyString = QString("Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\RedPanda-C++");
        QString value;
        if (!readRegistry(HKEY_LOCAL_MACHINE,keyString.toLocal8Bit(),"UninstallString",value)) {
            keyString = "SOFTWARE\\WOW6432Node\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\RedPanda-C++";
            if (!readRegistry(HKEY_LOCAL_MACHINE,keyString.toLocal8Bit(),"UninstallString",value)) {
                value="";
            }
        }
        if (!value.isEmpty()) {
            QString regPath = extractFileDir(value);

            QString appPath = QApplication::instance()->applicationDirPath();
            gIsGreenEdition = excludeTrailingPathDelimiter(regPath).compare(excludeTrailingPathDelimiter(appPath),
                                                                            Qt::CaseInsensitive)!=0;
        }
        gIsGreenEditionInited = true;
    }
    return gIsGreenEdition;
#else
    return false;
#endif
}

QByteArray runAndGetOutput(const QString &cmd, const QString& workingDir, const QStringList& arguments,
                           const QByteArray &inputContent, bool inheritEnvironment,
                           const QProcessEnvironment& env)
{
    QProcess process;
    QByteArray result;
    if (env.isEmpty()) {
        if (inheritEnvironment) {
            process.setProcessEnvironment(QProcessEnvironment::systemEnvironment());
        } else {
            process.setProcessEnvironment(QProcessEnvironment());
        }
    } else {
        process.setProcessEnvironment(env);
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
    if (!inputContent.isEmpty()) {
        process.write(inputContent);
    }
    process.closeWriteChannel();
    process.waitForFinished();
    return result;
}

bool isNonPrintableAsciiChar(char ch)
{
    return (ch<=32) && (ch>=0);
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
    if (filename.endsWith(".dev",PATH_SENSITIVITY)) {
        return FileType::Project;
    }
    if (filename.endsWith(".C")) {
        return FileType::CppSource;
    }
    if (filename.endsWith(".CPP")) {
        return FileType::CppSource;
    }
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
    if (filename.endsWith(".H")) {
        return FileType::CHeader;
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
    if (filename.endsWith(".rc",PATH_SENSITIVITY)) {
        return FileType::WindowsResourceSource;
    }
    return FileType::Other;
}


QString getCompiledExecutableName(const QString& filename)
{
    return changeFileExt(filename,EXECUTABLE_EXT);
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

bool programHasConsole(const QString & filename)
{
#ifdef Q_OS_WIN
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
#else
    return true;
#endif
}

QStringList textToLines(const QString &text)
{
    QTextStream stream(&((QString&)text),QIODevice::ReadOnly);
    return readStreamToLines(&stream);
}

QStringList readFileToLines(const QString& fileName, QTextCodec* codec)
{
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);
        stream.setCodec(codec);
        stream.setAutoDetectUnicode(false);
        return readStreamToLines(&stream);
    }
    return QStringList();
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

void textToLines(const QString &text, LineProcessFunc lineFunc)
{
    QTextStream stream(&((QString&)text),QIODevice::ReadOnly);
    readStreamToLines(&stream,lineFunc);
}

void readFileToLines(const QString &fileName, QTextCodec *codec, LineProcessFunc lineFunc)
{
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);
        stream.setCodec(codec);
        stream.setAutoDetectUnicode(false);
        readStreamToLines(&stream, lineFunc);
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

QString trimRight(const QString &s)
{
    if (s.isEmpty())
        return s;
    int i = s.length()-1;
//   while ((i>=0) && ((s[i] == '\r') || (s[i]=='\n') || (s[i] == '\t') || (s[i]==' ')))  {
    while ((i>=0) && (s[i]<=32)) {
        i--;
    };
    if (i>=0) {
        return s.left(i+1);
    } else {
        return QString();
    }
}

bool stringIsBlank(const QString &s)
{
    for (QChar ch:s) {
        if (ch != ' ' && ch != '\t')
            return false;
    }
    return true;
}

QString trimLeft(const QString &s)
{
    if (s.isEmpty())
        return s;
    int i=0;
//    while ((i<s.length()) && ((s[i] == '\r') || (s[i]=='\n') || (s[i] == '\t') || (s[i]==' ')))  {
//        i++;
//    };
    while ((i<s.length()) && (s[i]<=32))  {
        i++;
    };
    if (i<s.length()) {
        return s.mid(i);
    } else {
        return QString();
    }
}

//void changeTheme(const QString &themeName)
//{
//    if (themeName.isEmpty() || themeName == "default") {
//        QApplication::setStyle("Fusion");
//        QApplication* app = dynamic_cast<QApplication*>(QApplication::instance());
//        //app->setStyleSheet("");
//        return ;
//    }
//    QStyleFactory styleFactory;
//    if (styleFactory.keys().contains(themeName)) {
//        QApplication::setStyle(themeName);
//        QApplication* app = dynamic_cast<QApplication*>(QApplication::instance());
//        app->setStyleSheet("");
//        return;
//    }
//    QFile f(QString(":/themes/%1/style.qss").arg(themeName));

//    if (!f.exists())   {
//        qDebug()<<"Unable to set stylesheet, file not found\n";
//    } else {
//        QApplication::setStyle("Fusion");
//        f.open(QFile::ReadOnly | QFile::Text);
//        QTextStream ts(&f);
//        dynamic_cast<QApplication*>(QApplication::instance())->setStyleSheet(ts.readAll());
//    }
//}

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
        return path+filename+ext;
    } else {
        return path+fileInfo.completeBaseName()+ext;
    }
}

QStringList readFileToLines(const QString &fileName)
{
    QFile file(fileName);
    if (file.size()<=0)
        return QStringList();
    QTextCodec* codec = QTextCodec::codecForLocale();
    QStringList result;
    QTextCodec::ConverterState state;
    bool ok = true;
    if (file.open(QFile::ReadOnly)) {
        while (!file.atEnd()) {
            QByteArray array = file.readLine();
            QString s = codec->toUnicode(array,array.length(),&state);
            if (state.invalidChars>0) {
                ok=false;
                break;
            }
            result.append(s);
        }
        if (!ok) {
            file.seek(0);
            result.clear();
            codec = QTextCodec::codecForName("UTF-8");
            while (!file.atEnd()) {
                QByteArray array = file.readLine();
                QString s = codec->toUnicode(array,array.length(),&state);
                if (state.invalidChars>0) {
                    result.clear();
                    break;
                }
                result.append(s);
            }
        }
    }
    return result;
}

void stringsToFile(const QStringList &list, const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream stream(&file);
        for (QString s:list) {
            stream<<s<<endl;
        }
    }
}

void resetCppParser(std::shared_ptr<CppParser> parser, int compilerSetIndex)
{
    if (!parser)
        return;
    // Configure parser
    parser->reset();
    //paser->enabled = pSettings-> devCodeCompletion.Enabled;
//    CppParser.ParseLocalHeaders := devCodeCompletion.ParseLocalHeaders;
//    CppParser.ParseGlobalHeaders := devCodeCompletion.ParseGlobalHeaders;
    parser->setEnabled(true);
    parser->setParseGlobalHeaders(true);
    parser->setParseLocalHeaders(true);
    // Set options depending on the current compiler set
    // TODO: do this every time OnCompilerSetChanged
    if (compilerSetIndex<0) {
        compilerSetIndex=pSettings->compilerSets().defaultIndex();
    }
    Settings::PCompilerSet compilerSet = pSettings->compilerSets().getSet(compilerSetIndex);
    parser->clearIncludePaths();
    if (compilerSet) {
        foreach  (const QString& file,compilerSet->CppIncludeDirs()) {
            parser->addIncludePath(file);
        }
        foreach  (const QString& file,compilerSet->CIncludeDirs()) {
            parser->addIncludePath(file);
        }
        foreach  (const QString& file,compilerSet->defaultCppIncludeDirs()) {
            parser->addIncludePath(file);
        }
        foreach  (const QString& file,compilerSet->defaultCIncludeDirs()) {
            parser->addIncludePath(file);
        }
        //TODO: Add default include dirs last, just like gcc does
        // Set defines
        for (QString define:compilerSet->defines()) {
            parser->addHardDefineByLine(define); // predefined constants from -dM -E
        }
        // add a Red Pand C++ 's own macro
        parser->addHardDefineByLine("#define EGE_FOR_AUTO_CODE_COMPLETETION_ONLY");
        // add C/C++ default macro
        parser->addHardDefineByLine("#define __FILE__  1");
        parser->addHardDefineByLine("#define __LINE__  1");
        parser->addHardDefineByLine("#define __DATE__  1");
        parser->addHardDefineByLine("#define __TIME__  1");
    }
    parser->parseHardDefines();
    pMainWindow->disconnect(parser.get(),
                            &CppParser::onStartParsing,
                            pMainWindow,
                            &MainWindow::onStartParsing);
    pMainWindow->disconnect(parser.get(),
                            &CppParser::onProgress,
                            pMainWindow,
                            &MainWindow::onParserProgress);
    pMainWindow->disconnect(parser.get(),
                            &CppParser::onEndParsing,
                            pMainWindow,
                            &MainWindow::onEndParsing);
    pMainWindow->connect(parser.get(),
                            &CppParser::onStartParsing,
                            pMainWindow,
                            &MainWindow::onStartParsing);
    pMainWindow->connect(parser.get(),
                            &CppParser::onProgress,
                            pMainWindow,
                            &MainWindow::onParserProgress);
    pMainWindow->connect(parser.get(),
                            &CppParser::onEndParsing,
                            pMainWindow,
                            &MainWindow::onEndParsing);
}

bool findComplement(const QString &s, const QChar &fromToken, const QChar &toToken, int &curPos, int increment)
{
    int curPosBackup = curPos;
    int level = 0;
    //todo: skip comment, char and strings
    while ((curPos < s.length()) && (curPos >= 0)) {
        if (s[curPos] == fromToken) {
            level++;
        } else if (s[curPos] == toToken) {
            level--;
            if (level == 0)
                return true;
        }
        curPos += increment;
    }
    curPos = curPosBackup;
    return false;
}

void logToFile(const QString &s, const QString &filename, bool append)
{
    QFile file(filename);
    QFile::OpenMode mode = QFile::WriteOnly;
    if (append) {
        mode |= QFile::Append;
    } else {
        mode |= QFile::Truncate;
    }
    if (file.open(mode)) {
        QTextStream ts(&file);
        ts<<s<<endl;
    }
}

QString extractFileName(const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    return fileInfo.fileName();
}

QString extractRelativePath(const QString &base, const QString &dest)
{
    QFileInfo baseInfo(base);
    QDir baseDir;
    if (baseInfo.isDir()) {
        baseDir = QDir(baseInfo.absoluteFilePath());
    } else {
        baseDir = baseInfo.absoluteDir();
    }
    return baseDir.relativeFilePath(dest);
}

QString genMakePath(const QString &fileName, bool escapeSpaces, bool encloseInQuotes)
{
    QString result = fileName;

    // Convert backslashes to slashes
    result.replace('\\','/');
    if (escapeSpaces) {
        result.replace(' ',"\\ ");
    }
    if (encloseInQuotes)
        if (result.contains(' '))
            result = '"'+result+'"';
    return result;
}

QString genMakePath1(const QString &fileName)
{
    return genMakePath(fileName, false, true);
}

QString genMakePath2(const QString &fileName)
{
    return genMakePath(fileName, true, false);
}

QString getSizeString(int size)
{
    if (size < 1024) {
        return QString("%1 ").arg(size)+QObject::tr("bytes");
    } else if (size < 1024 * 1024) {
        return QString("%1 ").arg(size / 1024.0)+QObject::tr("KB");
    } else if (size < 1024 * 1024 * 1024) {
        return QString("%1 ").arg(size / 1024.0 / 1024.0)+QObject::tr("MB");
    } else {
        return QString("%1 ").arg(size / 1024.0 / 1024.0 / 1024.0)+QObject::tr("GB");
    }
}

int getNewFileNumber()
{
    static int count = 0;
    count++;
    return count;
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

QString extractFileDir(const QString &fileName)
{
    return extractFilePath(fileName);
}

QByteArray toByteArray(const QString &s)
{
    //return s.toLocal8Bit();
    return s.toUtf8();
}

QString fromByteArray(const QByteArray &s)
{
    QTextCodec* codec = QTextCodec::codecForName(ENCODING_UTF8);
    QTextCodec::ConverterState state;
    if (!codec)
        return QString(s);
    QString tmp = codec->toUnicode(s,s.length(),&state);
    if (state.invalidChars>0)
        tmp = QString::fromLocal8Bit(s);
    return tmp;
}

QString linesToText(const QStringList &lines)
{
    return lines.join("\n");
}

QString parseMacros(const QString &s)
{
    QString result = s;
    Editor *e = pMainWindow->editorList()->getEditor();

    result.replace("<DEFAULT>", localizePath(QDir::currentPath()));
    result.replace("<DEVCPP>", localizePath(pSettings->dirs().executable()));
    result.replace("<DEVCPPVERSION>", REDPANDA_CPP_VERSION);
    result.replace("<EXECPATH>", localizePath(pSettings->dirs().appDir()));
    QDate today = QDate::currentDate();
    QDateTime now = QDateTime::currentDateTime();

    result.replace("<DATE>", today.toString("yyyy-MM-dd"));
    result.replace("<DATETIME>", now.toString("yyyy-MM-dd hh:mm:ss"));

    Settings::PCompilerSet compilerSet = pSettings->compilerSets().defaultSet();
    if (compilerSet) {
        // Only provide the first cpp include dir
        if (compilerSet->defaultCppIncludeDirs().count()>0)
            result.replace("<INCLUDE>", localizePath(compilerSet->defaultCppIncludeDirs().front()));
        else
            result.replace("<INCLUDE>","");

        // Only provide the first lib dir
        if (compilerSet->defaultLibDirs().count()>0)
            result.replace("<LIB>", localizePath(compilerSet->defaultLibDirs().front()));
        else
            result.replace("<LIB>","");
    }

    if (e!=nullptr && !e->inProject()) { // Non-project editor macros
            result.replace("<EXENAME>", extractFileName(changeFileExt(e->filename(),EXECUTABLE_EXT)));
            result.replace("<EXEFILE>", localizePath(changeFileExt(e->filename(),EXECUTABLE_EXT)));
            result.replace("<PROJECTNAME>", extractFileName(e->filename()));
            result.replace("<PROJECTFILE>", localizePath(e->filename()));
            result.replace("<PROJECTFILENAME>", extractFileName(e->filename()));
            result.replace("<PROJECTPATH>", localizePath(extractFileDir(e->filename())));
    } else if (pMainWindow->project()) {
        result.replace("<EXENAME>", extractFileName(pMainWindow->project()->executable()));
        result.replace("<EXEFILE>", localizePath(pMainWindow->project()->executable()));
        result.replace("<PROJECTNAME>", pMainWindow->project()->name());
        result.replace("<PROJECTFILE>", localizePath(pMainWindow->project()->filename()));
        result.replace("<PROJECTFILENAME>", extractFileName(pMainWindow->project()->filename()));
        result.replace("<PROJECTPATH>", localizePath(pMainWindow->project()->directory()));
    } else {
        result.replace("<EXENAME>", "");
        result.replace("<EXEFILE>", "");
        result.replace("<PROJECTNAME>", "");
        result.replace("<PROJECTFILE>", "");
        result.replace("<PROJECTFILENAME>", "");
        result.replace("<PROJECTPATH>", "");
    }

    // Editor macros
    if (e!=nullptr) {
        result.replace("<SOURCENAME>", extractFileName(e->filename()));
        result.replace("<SOURCEFILE>", localizePath(e->filename()));
        result.replace("<SOURCEPATH>", localizePath(extractFileDir(e->filename())));
        result.replace("<WORDXY>", e->wordAtCursor());
    } else {
        result.replace("<SOURCENAME>", "");
        result.replace("<SOURCEFILE>", "");
        result.replace("<SOURCEPATH>", "");
        result.replace("<WORDXY>", "");
    }
    return result;
}

void executeFile(const QString &fileName, const QString &params, const QString &workingDir, const QString &tempFile)
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

void stringToFile(const QString &str, const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        QTextStream stream(&file);
        stream<<str;
    }
}

bool removeFile(const QString &filename)
{
    QFile file(filename);
    return file.remove();
}

QByteArray readFileToByteArray(const QString &fileName)
{
    QFile file(fileName);
    if (file.open(QFile::ReadOnly)) {
        return file.readAll();
    }
    return QByteArray();
}

QByteArray getHTTPBody(const QByteArray& content) {
    int i= content.indexOf("\r\n\r\n");
    if (i>=0) {
        return content.mid(i+4);
    }
    return "";
}

bool haveGoodContrast(const QColor& c1, const QColor &c2) {
    int lightness1 = qGray(c1.rgb());
    int lightness2 = qGray(c2.rgb());
    return std::abs(lightness1 - lightness2)>=120;
}


#ifdef Q_OS_WIN
bool readRegistry(HKEY key,const QByteArray& subKey, const QByteArray& name, QString& value) {
    DWORD dataSize;
    LONG result;
    result = RegGetValueA(key,subKey,
                 name, RRF_RT_REG_SZ | RRF_RT_REG_MULTI_SZ,
                 NULL,
                 NULL,
                 &dataSize);
    if (result!=ERROR_SUCCESS)
        return false;
    char * buffer = new char[dataSize+10];
    result = RegGetValueA(key,subKey,
                 name, RRF_RT_REG_SZ | RRF_RT_REG_MULTI_SZ,
                 NULL,
                 buffer,
                 &dataSize);
    if (result!=ERROR_SUCCESS) {
        delete[] buffer;
        return false;
    }
    value=QString::fromLocal8Bit(buffer);
    delete [] buffer;
    return true;
}
#endif

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

QString localizePath(const QString &path)
{
    QString result = path;
    result.replace("/",QDir::separator());
    return result;
}

float pointToPixel(float point)
{
    return point * screenDPI() / 72;
}

float pixelToPoint(float pixel)
{
    return pixel * 72 / screenDPI();
}


QStringList splitProcessCommand(const QString &cmd)
{
    QStringList result;
    SplitProcessCommandQuoteType quoteType = SplitProcessCommandQuoteType::None;
    int i=0;
    QString current;
    while (i<cmd.length()) {
        switch (cmd[i].unicode()) {
        case ' ':
        case '\t':
        case '\r':
        case '\n':
            if (quoteType == SplitProcessCommandQuoteType::None) {
                if (!current.isEmpty()) {
                    result.append(current);
                }
                current = "";
            } else {
                current += cmd[i];
            }
            i++;
            break;
        case '\"':
            switch(quoteType) {
            case SplitProcessCommandQuoteType::None:
                quoteType = SplitProcessCommandQuoteType::Double;
                break;
            case SplitProcessCommandQuoteType::Double:
                quoteType = SplitProcessCommandQuoteType::None;
                break;
            default:
                current+=cmd[i];
            }
            i++;
            break;
        case '\'':
            switch(quoteType) {
            case SplitProcessCommandQuoteType::None:
                quoteType = SplitProcessCommandQuoteType::Single;
                break;
            case SplitProcessCommandQuoteType::Single:
                quoteType = SplitProcessCommandQuoteType::None;
                break;
            default:
                current+=cmd[i];
            }
            i++;
            break;
        case '\\':
            current += cmd[i];
            i++;
            if  (i<cmd.length()) {
                current += cmd[i];
                i++;
            }
            break;
        default:
            current += cmd[i];
            i++;
        }
    }
    if (!current.isEmpty())
        result.append(current);
    return result;
}

static int defaultScreenDPI = -1;

int screenDPI()
{
    if (defaultScreenDPI<1) {
        defaultScreenDPI = qApp->primaryScreen()->logicalDotsPerInch();
    }
    return defaultScreenDPI;
}

qulonglong stringToHex(const QString &str, qulonglong defaultValue)
{
    bool isOk;
    qulonglong value = str.toULongLong(&isOk,16);
    if (isOk)
        return value;
    return defaultValue;
}

void setScreenDPI(int dpi)
{
    defaultScreenDPI = dpi;
}
