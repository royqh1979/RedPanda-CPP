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

#ifdef Q_OS_WIN
using pIsWow64Process2_t = BOOL (WINAPI *)(
    HANDLE hProcess, USHORT *pProcessMachine, USHORT *pNativeMachine
);
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
    HANDLE handle = CreateFileW(filename.toStdWString().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
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
        QString appPath = QApplication::instance()->applicationDirPath();
        appPath = excludeTrailingPathDelimiter(appPath);
        QString keyString = R"(Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++)";
        QString systemInstallPath;
        readRegistry(HKEY_LOCAL_MACHINE, keyString, "UninstallString", systemInstallPath);
        if (!systemInstallPath.isEmpty())
            systemInstallPath = excludeTrailingPathDelimiter(extractFileDir(systemInstallPath));
        QString userInstallPath;
        readRegistry(HKEY_CURRENT_USER, keyString, "UninstallString", userInstallPath);
        if (!userInstallPath.isEmpty())
            userInstallPath = excludeTrailingPathDelimiter(extractFileDir(userInstallPath));
        gIsGreenEdition = appPath.compare(systemInstallPath, Qt::CaseInsensitive) != 0 &&
                appPath.compare(userInstallPath, Qt::CaseInsensitive) != 0;
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
                splitProcessCommand(params),
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
bool readRegistry(HKEY key,const QString& subKey, const QString& name, QString& value) {
    LONG result;
    HKEY hkey;
    result = RegOpenKeyExW(key, subKey.toStdWString().c_str(), 0, KEY_READ, &hkey);
    if (result != ERROR_SUCCESS)
        return false;

    DWORD dataType;
    DWORD dataSize;
    result = RegQueryValueExW(hkey, name.toStdWString().c_str(), NULL, &dataType, NULL, &dataSize);
    if (result != ERROR_SUCCESS || (dataType != REG_SZ && dataType != REG_MULTI_SZ)) {
        RegCloseKey(hkey);
        return false;
    }

    wchar_t * buffer = new wchar_t[dataSize / sizeof(wchar_t) + 10];
    result = RegQueryValueExW(hkey, name.toStdWString().c_str(), NULL, &dataType, (LPBYTE)buffer, &dataSize);
    RegCloseKey(hkey);
    if (result != ERROR_SUCCESS) {
        delete[] buffer;
        return false;
    }

    value = QString::fromWCharArray(buffer);
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

QString escapeArgument(const QString &arg, [[maybe_unused]] bool isFirstArg)
{
    auto argContainsOneOf = [&arg](auto... ch) { return (arg.contains(ch) || ...); };

#ifdef Q_OS_WINDOWS
    // See https://stackoverflow.com/questions/31838469/how-do-i-convert-argv-to-lpcommandline-parameter-of-createprocess ,
    // and https://learn.microsoft.com/en-gb/archive/blogs/twistylittlepassagesallalike/everyone-quotes-command-line-arguments-the-wrong-way .

    // TODO: investigate whether we need escaping for cmd.

    if (!arg.isEmpty() && !argContainsOneOf(' ', '\t', '\n', '\v', '"'))
        return arg;

    QString result = "\"";
    for (auto it = arg.begin(); ; ++it) {
        int nBackslash = 0;
        while (it != arg.end() && *it == '\\') {
            ++it;
            ++nBackslash;
        }
        if (it == arg.end()) {
            // Escape all backslashes, but let the terminating double quotation mark we add below be interpreted as a metacharacter.
            result.append(QString('\\').repeated(nBackslash * 2));
            break;
        } else if (*it == '"') {
            // Escape all backslashes and the following double quotation mark.
            result.append(QString('\\').repeated(nBackslash * 2 + 1));
            result.push_back(*it);
        } else {
            // Backslashes aren't special here.
            result.append(QString('\\').repeated(nBackslash));
            result.push_back(*it);
        }
    }
    result.push_back('"');
    return result;
#else
    /* be speculative, but keep readability.
     * ref. https://pubs.opengroup.org/onlinepubs/9699919799.2018edition/utilities/V3_chap02.html
     */
    if (arg.isEmpty())
        return R"("")";

    /* POSIX say the following reserved words (may) have special meaning:
     *   !, {, }, case, do, done, elif, else, esac, fi, for, if, in, then, until, while,
     *   [[, ]], function, select,
     * only if used as the _first word_ of a command (or somewhere we dot not care).
     */
    const static QSet<QString> reservedWord{
        "!", "{", "}", "case", "do", "done", "elif", "else", "esac", "fi", "for", "if", "in", "then", "until", "while",
        "[[", "]]", "function", "select",
    };
    if (isFirstArg && reservedWord.contains(arg))
        return QString(R"("%1")").arg(arg);

    /* POSIX say “shall quote”:
     *   '|', '&', ';', '<', '>', '(', ')', '$', '`', '\\', '"', '\'', ' ', '\t', '\n';
     * and “may need to be quoted”:
     *   '*', '?', '[', '#', '~', '=', '%'.
     * among which “may need to be quoted” there are 4 kinds:
     *   - wildcards '*', '?', '[' are “danger anywhere” (handle it as if “shall quote”);
     *   - comment '#', home '~', is “danger at first char in any word”;
     *   - (environment) variable '=' is “danger at any char in first word”;
     *   - foreground '%' is “danger at first char in first word”.
     * although not mentioned by POSIX, bash’s brace expansion '{', '}' are also “danger anywhere”.
     */
    bool isDoubleQuotingDanger = argContainsOneOf('$', '`', '\\', '"');
    bool isSingleQuotingDanger = arg.contains('\'');
    bool isDangerAnyChar = isDoubleQuotingDanger || isSingleQuotingDanger || argContainsOneOf(
        '|', '&', ';', '<', '>', '(', ')', ' ', '\t', '\n',
        '*', '?', '[',
        '{', '}'
    );
    bool isDangerFirstChar = (arg[0] == '#') || (arg[0] == '~');
    if (isFirstArg) {
        isDangerAnyChar = isDangerAnyChar || arg.contains('=');
        isDangerFirstChar = isDangerFirstChar || (arg[0] == '%');
    }

    // a “safe” string
    if (!isDangerAnyChar && !isDangerFirstChar)
        return arg;

    // prefer more-common double quoting
    if (!isDoubleQuotingDanger)
        return QString(R"("%1")").arg(arg);

    // and then check the opportunity of single quoting
    if (!isSingleQuotingDanger)
        return QString("'%1'").arg(arg);

    // escaping is necessary
    // use double quoting since it’s less tricky
    QString result = "\"";
    for (auto ch : arg) {
        if (ch == '$' || ch == '`' || ch == '\\' || ch == '"')
            result.push_back('\\');
        result.push_back(ch);
    }
    result.push_back('"');
    return result;

    /* single quoting, which is roughly raw string, is possible and quite simple in programming:
     *   1. replace each single quote with `'\''`, which contains
     *      - a single quote to close quoting,
     *      - an escaped single quote representing the single quote itself, and
     *      - a single quote to open quoting again;
     *   2. enclose the string with a pair of single quotes.
     * e.g. `o'clock` => `'o'\''clock'`, really tricky and hard to read.
     */
#endif
}

QString defaultShell()
{
#ifdef Q_OS_WINDOWS
    return "powershell.exe";
#else
    return "sh";
#endif
}

QString appArch()
{
#ifdef _M_ARM64EC
    return "arm64ec";
#else
    return QSysInfo::buildCpuArchitecture();
#endif
}

QString osArch()
{
#ifdef Q_OS_WINDOWS
    pIsWow64Process2_t pIsWow64Process2 =
        reinterpret_cast<pIsWow64Process2_t>(GetProcAddress(GetModuleHandleW(L"kernel32"), "IsWow64Process2"));
    if (pIsWow64Process2) {
        // IsWow64Process2 returns real native architecture under xtajit, while GetNativeSystemInfo does not.
        USHORT processMachineResult, nativeMachineResult;
        if (pIsWow64Process2(GetCurrentProcess(), &processMachineResult, &nativeMachineResult))
            switch (nativeMachineResult) {
            case IMAGE_FILE_MACHINE_I386:
                return "i386";
            case IMAGE_FILE_MACHINE_AMD64:
                return "x86_64";
            case IMAGE_FILE_MACHINE_ARM64:
                return "arm64";
            }
        return QSysInfo::currentCpuArchitecture();
    } else
        return QSysInfo::currentCpuArchitecture();
#else
    return QSysInfo::currentCpuArchitecture();
#endif
}
