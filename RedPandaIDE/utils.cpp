#include "utils.h"
#include "systemconsts.h"
#include <QDate>
#include <QDateTime>
#include <QApplication>
#include <QDesktopServices>
#include <QSysInfo>
#include <QVersionNumber>
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
    if (filename.endsWith(".def",PATH_SENSITIVITY)) {
        return FileType::ModuleDef;
    }
    QFileInfo info(filename);
    if (info.suffix().isEmpty()) {
        return FileType::Other;
    }
    return FileType::Other;
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
    return parseMacros(s, devCppMacroVariables());
}

QString parseMacros(const QString &s, const QMap<QString, QString> &macros)
{
    QString result = s;
    for (auto it = macros.begin(); it != macros.end(); ++it) {
        QString key = it.key();
        QString value = it.value();
        result.replace('<' + key + '>', value);
    }
    return result;
}

QMap<QString, QString> devCppMacroVariables()
{
    Editor *e = pMainWindow->editorList()->getEditor();

    QMap<QString, QString> result = {
        {"DEFAULT", localizePath(QDir::currentPath())},
        {"DEVCPP", localizePath(pSettings->dirs().executable())},
        {"DEVCPPVERSION", REDPANDA_CPP_VERSION},
        {"EXECPATH", localizePath(pSettings->dirs().appDir())},
        {"DATE", QDate::currentDate().toString("yyyy-MM-dd")},
        {"DATETIME", QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss")}
    };

    Settings::PCompilerSet compilerSet = pSettings->compilerSets().defaultSet();
    if (compilerSet) {
        // Only provide the first cpp include dir
        if (compilerSet->defaultCppIncludeDirs().count() > 0)
            result["INCLUDE"] = localizePath(compilerSet->defaultCppIncludeDirs().front());
        else
            result["INCLUDE"] = "";

        // Only provide the first lib dir
        if (compilerSet->defaultLibDirs().count() > 0)
            result["LIB"] = localizePath(compilerSet->defaultLibDirs().front());
        else
            result["LIB"] = "";
    }

    if (e != nullptr && !e->inProject()) { // Non-project editor macros
        QString exeSuffix;
        Settings::PCompilerSet compilerSet = pSettings->compilerSets().defaultSet();
        if (compilerSet) {
            exeSuffix = compilerSet->executableSuffix();
        } else {
            exeSuffix = DEFAULT_EXECUTABLE_SUFFIX;
        }
        result["EXENAME"] = extractFileName(changeFileExt(e->filename(), exeSuffix));
        result["EXEFILE"] = localizePath(changeFileExt(e->filename(), exeSuffix));
        result["PROJECTNAME"] = extractFileName(e->filename());
        result["PROJECTFILE"] = localizePath(e->filename());
        result["PROJECTFILENAME"] = extractFileName(e->filename());
        result["PROJECTPATH"] = localizePath(extractFileDir(e->filename()));
    } else if (pMainWindow->project()) {
        result["EXENAME"] = extractFileName(pMainWindow->project()->executable());
        result["EXEFILE"] = localizePath(pMainWindow->project()->executable());
        result["PROJECTNAME"] = pMainWindow->project()->name();
        result["PROJECTFILE"] = localizePath(pMainWindow->project()->filename());
        result["PROJECTFILENAME"] = extractFileName(pMainWindow->project()->filename());
        result["PROJECTPATH"] = localizePath(pMainWindow->project()->directory());
    } else {
        result["EXENAME"] = "";
        result["EXEFILE"] = "";
        result["PROJECTNAME"] = "";
        result["PROJECTFILE"] = "";
        result["PROJECTFILENAME"] = "";
        result["PROJECTPATH"] = "";
    }

    // Editor macros
    if (e != nullptr) {
        result["SOURCENAME"] = extractFileName(e->filename());
        result["SOURCEFILE"] = localizePath(e->filename());
        result["SOURCEPATH"] = localizePath(extractFileDir(e->filename()));
        result["WORDXY"] = e->wordAtCursor();
    } else {
        result["SOURCENAME"] = "";
        result["SOURCEFILE"] = "";
        result["SOURCEPATH"] = "";
        result["WORDXY"] = "";
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
        appPath = excludeTrailingPathDelimiter(localizePath(appPath));
        QString keyString = R"(Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++)";
        QString systemInstallPath;
        readRegistry(HKEY_LOCAL_MACHINE, keyString, "InstallLocation", systemInstallPath);
        if (!systemInstallPath.isEmpty())
            systemInstallPath = excludeTrailingPathDelimiter(systemInstallPath);
        QString userInstallPath;
        readRegistry(HKEY_CURRENT_USER, keyString, "InstallLocation", userInstallPath);
        if (!userInstallPath.isEmpty())
            userInstallPath = excludeTrailingPathDelimiter(userInstallPath);
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
