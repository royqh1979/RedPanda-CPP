#include "utils.h"
#include "systemconsts.h"
#include <QDate>
#include <QDateTime>
#include <QApplication>
#include <QDesktopServices>
#include "editor.h"
#include "editorlist.h"
#include "settings.h"
#include "mainwindow.h"
#include "project.h"
#include "parser/cppparser.h"
#include "compiler/executablerunner.h"
#include <QComboBox>
#ifdef Q_OS_WIN
#include <QDesktopServices>
#include <windows.h>
#endif

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

FileType getFileType(const QString &filename)
{
    if (filename.endsWith(".s",PATH_SENSITIVITY)) {
        return FileType::GAS;
    }
    if (filename.endsWith(".S",PATH_SENSITIVITY)) {
        return FileType::GAS;
    }
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
    if (filename.endsWith(".in",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".out",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".txt",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".md",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".info",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".dat",PATH_SENSITIVITY)) {
        return FileType::Text;
    }
    if (filename.endsWith(".lua",PATH_SENSITIVITY)) {
        return FileType::LUA;
    }
    if (filename.endsWith(".fs",PATH_SENSITIVITY)) {
        return FileType::FragmentShader;
    }
    if (filename.endsWith(".vs",PATH_SENSITIVITY)) {
        return FileType::VerticeShader;
    }
    QFileInfo info(filename);
    if (info.suffix().isEmpty()) {
        return FileType::Other;
    }
    return FileType::Other;
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
        QString exeSuffix;
        Settings::PCompilerSet compilerSet = pSettings->compilerSets().defaultSet();
        if (compilerSet) {
            exeSuffix = compilerSet->executableSuffix();
        } else {
            exeSuffix = DEFAULT_EXECUTABLE_SUFFIX;
        }
        result.replace("<EXENAME>", extractFileName(changeFileExt(e->filename(), exeSuffix)));
        result.replace("<EXEFILE>", localizePath(changeFileExt(e->filename(), exeSuffix)));
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

void resetCppParser(std::shared_ptr<CppParser> parser, int compilerSetIndex)
{
    if (!parser)
        return;
    // Configure parser
    parser->resetParser();
    //paser->enabled = pSettings-> devCodeCompletion.Enabled;
//    CppParser.ParseLocalHeaders := devCodeCompletion.ParseLocalHeaders;
//    CppParser.ParseGlobalHeaders := devCodeCompletion.ParseGlobalHeaders;
    parser->setEnabled(true);
    parser->setParseGlobalHeaders(true);
    parser->setParseLocalHeaders(true);
    // Set options depending on the current compiler set
    if (compilerSetIndex<0) {
        compilerSetIndex=pSettings->compilerSets().defaultIndex();
    }
    Settings::PCompilerSet compilerSet = pSettings->compilerSets().getSet(compilerSetIndex);
    parser->clearIncludePaths();
    bool isCpp = parser->language()==ParserLanguage::CPlusPlus;
    if (compilerSet) {
        if (isCpp) {
            foreach  (const QString& file,compilerSet->CppIncludeDirs()) {
                parser->addIncludePath(file);
            }
        }
        foreach  (const QString& file,compilerSet->CIncludeDirs()) {
            parser->addIncludePath(file);
        }
        if (isCpp) {
            foreach  (const QString& file,compilerSet->defaultCppIncludeDirs()) {
                parser->addIncludePath(file);
            }
        }
        foreach  (const QString& file,compilerSet->defaultCIncludeDirs()) {
            parser->addIncludePath(file);
        }
        // Set defines
        for (QString define:compilerSet->defines(parser->language()==ParserLanguage::CPlusPlus)) {
            parser->addHardDefineByLine(define);
        }
//        // add a Red Pand C++ 's own macro
//        parser->addHardDefineByLine("#define EGE_FOR_AUTO_CODE_COMPLETETION_ONLY");
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

int getNewFileNumber()
{
    static int count = 0;
    count++;
    return count;
}

#ifdef Q_OS_WIN
static bool gIsGreenEdition = true;
static bool gIsGreenEditionInited = false;
#endif
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

qulonglong stringToHex(const QString &str, bool &isOk)
{
    qulonglong value = str.toULongLong(&isOk,16);
    return value;
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

bool haveGoodContrast(const QColor& c1, const QColor &c2) {
    int lightness1 = qGray(c1.rgb());
    int lightness2 = qGray(c2.rgb());
    return std::abs(lightness1 - lightness2)>=120;
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

void saveComboHistory(QComboBox* cb,const QString& text) {
    QString s = text;
    if (s.isEmpty())
        return;
    int i = cb->findText(s);
    if (i>=0) {
        cb->removeItem(i);
    }
    cb->insertItem(0,s);
    cb->setCurrentText(s);
}

void openFileFolderInExplorer(const QString &path)
{
    QFileInfo info(path);
    if (info.isFile()){
#ifdef Q_OS_WIN
        QProcess process;
        QStringList args;
        QString filepath=info.absoluteFilePath().replace("/","\\");
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

QColor alphaBlend(const QColor &lower, const QColor &upper) {
    qreal wu = upper.alphaF(); // weight of upper color
    qreal wl = 1 - wu;         // weight of lower color
    return QColor(
        int(lower.red() * wl + upper.red() * wu),
        int(lower.green() * wl + upper.green() * wu),
        int(lower.blue() * wl + upper.blue() * wu)
        );
}
