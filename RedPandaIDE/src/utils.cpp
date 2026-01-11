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

#ifdef Q_OS_WIN
using pIsWow64Process2_t = BOOL (WINAPI *)(
    HANDLE hProcess, USHORT *pProcessMachine, USHORT *pNativeMachine
);
#endif

NonExclusiveTemporaryFileOwner::NonExclusiveTemporaryFileOwner(std::unique_ptr<QTemporaryFile> &tempFile) :
    filename(tempFile ? tempFile->fileName() : QString())
{
    if (tempFile) {
        tempFile->flush();
        tempFile->setAutoRemove(false);
        tempFile = nullptr;
    }
}

NonExclusiveTemporaryFileOwner::~NonExclusiveTemporaryFileOwner()
{
    if (!filename.isEmpty())
        QFile::remove(filename);
}

FileType getFileType(const QString &filename)
{
    if (filename.isEmpty())
        return FileType::None;
    if (filename.startsWith("makefile", PATH_SENSITIVITY)) {
        return FileType::MakeFile;
    }
    if (filename.endsWith(".s",PATH_SENSITIVITY)) {
        return FileType::GAS;
    }
    if (filename.endsWith(".S",PATH_SENSITIVITY)) {
        return FileType::GAS;
    }
    if (filename.endsWith(".asm",PATH_SENSITIVITY)) {
        return FileType::NASM;
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
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".h",PATH_SENSITIVITY)) {
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".hpp",PATH_SENSITIVITY)) {
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".hh",PATH_SENSITIVITY)) {
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".hxx",PATH_SENSITIVITY)) {
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".tcc",PATH_SENSITIVITY)) {
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".inl",PATH_SENSITIVITY)) {
        return FileType::CCppHeader;
    }
    if (filename.endsWith(".gimple",PATH_SENSITIVITY)) {
        return FileType::GIMPLE;
    }
    if (filename.endsWith(".p",PATH_SENSITIVITY)) {
        return FileType::PreprocessedSource;
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

bool programIsWin32GuiApp(const QString & filename)
{
#ifdef Q_OS_WIN
    bool result = false;
    HANDLE handle = CreateFileW(filename.toStdWString().c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
    if (handle != INVALID_HANDLE_VALUE) {
        result = [handle] {
            IMAGE_DOS_HEADER dos_header;
            DWORD signature;
            DWORD bytesread;
            IMAGE_FILE_HEADER pe_header;
            IMAGE_OPTIONAL_HEADER opt_header;

            ReadFile(handle, &dos_header, sizeof(dos_header), &bytesread, NULL);
            if (bytesread != sizeof(dos_header))
                return false;
            if (dos_header.e_magic != IMAGE_DOS_SIGNATURE)
                return false;

            SetFilePointer(handle, dos_header.e_lfanew, NULL, 0);
            ReadFile(handle, &signature, sizeof(signature), &bytesread, NULL);
            if (bytesread != sizeof(signature))
                return false;
            if (signature != IMAGE_NT_SIGNATURE)
                return false;

            ReadFile(handle, &pe_header, sizeof(pe_header), &bytesread, NULL);
            if (bytesread != sizeof(pe_header))
                return false;

            ReadFile(handle, &opt_header, sizeof(opt_header), &bytesread, NULL);
            if (bytesread != sizeof(opt_header))
                return false;
            if (opt_header.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC && opt_header.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                return false;

            return opt_header.Subsystem == IMAGE_SUBSYSTEM_WINDOWS_GUI;
        } ();
    }
    CloseHandle(handle);
    return result;
#else
    return false;
#endif
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

bool isGreenEdition()
{
    if (!gIsGreenEditionInited) {
        QString appPath = QApplication::instance()->applicationDirPath();
        appPath = excludeTrailingPathDelimiter(localizePath(appPath));
        QString keyString = R"(Software\Microsoft\Windows\CurrentVersion\Uninstall\RedPanda-C++)";
        QString systemInstallPath;
        readRegistry(HKEY_LOCAL_MACHINE, keyString, "InstallLocation", systemInstallPath);
        if (systemInstallPath.isEmpty()) {
            readRegistry(HKEY_LOCAL_MACHINE, keyString, "UninstallString", systemInstallPath);
            systemInstallPath = excludeTrailingPathDelimiter(extractFileDir(systemInstallPath));
        } else
            systemInstallPath = excludeTrailingPathDelimiter(systemInstallPath);
        QString userInstallPath;
        readRegistry(HKEY_CURRENT_USER, keyString, "InstallLocation", userInstallPath);
        if (userInstallPath.isEmpty()) {
            readRegistry(HKEY_CURRENT_USER, keyString, "UninstallString", userInstallPath);
            userInstallPath = excludeTrailingPathDelimiter(extractFileDir(userInstallPath));
        } else
            userInstallPath = excludeTrailingPathDelimiter(userInstallPath);
        systemInstallPath = localizePath(systemInstallPath);
        userInstallPath = localizePath(userInstallPath);
        gIsGreenEdition = appPath.compare(systemInstallPath, Qt::CaseInsensitive) != 0 &&
                appPath.compare(userInstallPath, Qt::CaseInsensitive) != 0;
        gIsGreenEditionInited = true;
    }
    return gIsGreenEdition;
}
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
    return QSysInfo::buildCpuArchitecture();
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

std::tuple<QString, QStringList, PNonExclusiveTemporaryFileOwner> wrapCommandForTerminalEmulator(const QString &terminal, const QStringList &argsPattern, const QStringList &payloadArgsWithArgv0)
{
    QStringList wrappedArgs;
    std::unique_ptr<QTemporaryFile> temproryFile;
    for (const QString &patternItem : argsPattern) {
        if (patternItem == "$term")
            wrappedArgs.append(terminal);
        else if (patternItem == "$integrated_term")
            wrappedArgs.append(includeTrailingPathDelimiter(pSettings->dirs().appDir())+terminal);
        else if (patternItem == "$argv")
            wrappedArgs.append(payloadArgsWithArgv0);
        else if (patternItem == "$command" || patternItem == "$unix_command") {
            // “$command” is for compatibility; previously used on multiple Unix terms
            QStringList escapedArgs;
            for (int i = 0; i < payloadArgsWithArgv0.length(); i++) {
                auto &arg = payloadArgsWithArgv0[i];
                auto escaped = escapeArgument(arg, i == 0, EscapeArgumentRule::BourneAgainShellPretty);
                escapedArgs.append(escaped);
            }
            wrappedArgs.push_back(escapedArgs.join(' '));
        } else if (patternItem == "$dos_command") {
            QStringList escapedArgs;
            for (int i = 0; i < payloadArgsWithArgv0.length(); i++) {
                auto &arg = payloadArgsWithArgv0[i];
                auto escaped = escapeArgument(arg, i == 0, EscapeArgumentRule::WindowsCommandPrompt);
                escapedArgs.append(escaped);
            }
            wrappedArgs.push_back(escapedArgs.join(' '));
        } else if (patternItem == "$lpCommandLine") {
            QStringList escapedArgs;
            for (int i = 0; i < payloadArgsWithArgv0.length(); i++) {
                auto &arg = payloadArgsWithArgv0[i];
                auto escaped = escapeArgument(arg, i == 0, EscapeArgumentRule::WindowsCreateProcess);
                escapedArgs.append(escaped);
            }
            wrappedArgs.push_back(escapedArgs.join(' '));
        } else if (patternItem == "$tmpfile" || patternItem == "$tmpfile.command") {
            // “$tmpfile” is for compatibility; previously used on macOS Terminal.app
            temproryFile = std::make_unique<QTemporaryFile>(QDir::tempPath() + "/redpanda_XXXXXX.command");
            if (temproryFile->open()) {
                QStringList escapedArgs;
                for (int i = 0; i < payloadArgsWithArgv0.length(); i++) {
                    auto &arg = payloadArgsWithArgv0[i];
                    auto escaped = escapeArgument(arg, i == 0, EscapeArgumentRule::BourneAgainShellPretty);
                    escapedArgs.append(escaped);
                }
                temproryFile->write(escapedArgs.join(' ').toUtf8());
                temproryFile->write("\n");
                QFile(temproryFile->fileName()).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
            }
            wrappedArgs.push_back(temproryFile->fileName());
        } else if (patternItem == "$tmpfile.sh") {
            temproryFile = std::make_unique<QTemporaryFile>(QDir::tempPath() + "/redpanda_XXXXXX.command");
            if (temproryFile->open()) {
                QStringList escapedArgs = {"exec"};
                for (int i = 0; i < payloadArgsWithArgv0.length(); i++) {
                    auto &arg = payloadArgsWithArgv0[i];
                    auto escaped = escapeArgument(arg, false, EscapeArgumentRule::BourneAgainShellPretty);
                    escapedArgs.append(escaped);
                }
                temproryFile->write("#!/bin/sh\n");
                temproryFile->write(escapedArgs.join(' ').toUtf8());
                temproryFile->write("\n");
                QFile(temproryFile->fileName()).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
            }
            wrappedArgs.push_back(temproryFile->fileName());
        } else if (patternItem == "$tmpfile.bat") {
            temproryFile = std::make_unique<QTemporaryFile>(QDir::tempPath() + "/redpanda_XXXXXX.bat");
            if (temproryFile->open()) {
                QStringList escapedArgs;
                for (int i = 0; i < payloadArgsWithArgv0.length(); i++) {
                    auto &arg = payloadArgsWithArgv0[i];
                    auto escaped = escapeArgument(arg, i == 0, EscapeArgumentRule::WindowsCommandPrompt);
                    escapedArgs.append(escaped);
                }
                temproryFile->write(escapedArgs.join(' ').toLocal8Bit());
                temproryFile->write("\r\n");
                QFile(temproryFile->fileName()).setPermissions(QFileDevice::ReadOwner | QFileDevice::WriteOwner | QFileDevice::ExeOwner);
            }
            wrappedArgs.push_back(temproryFile->fileName());
        } else if (patternItem == "$sequential_app_id") {
            static QString prefix = QStringLiteral("io.redpanda.term_%1_").arg(QCoreApplication::applicationPid());
            static std::atomic<int> appIdCounter = 0;
            QString appId = prefix + QString::number(++appIdCounter);
            wrappedArgs.push_back(appId);
        } else
            wrappedArgs.push_back(patternItem);
    }
    if (wrappedArgs.empty())
        return {QString(""), QStringList{}, std::make_unique<NonExclusiveTemporaryFileOwner>(temproryFile)};
    return {wrappedArgs[0], wrappedArgs.mid(1), std::make_unique<NonExclusiveTemporaryFileOwner>(temproryFile)};
}

std::tuple<QString, QStringList, PNonExclusiveTemporaryFileOwner> wrapCommandForTerminalEmulator(const QString &terminal, const QString &argsPattern, const QStringList &payloadArgsWithArgv0)
{
    return wrapCommandForTerminalEmulator(terminal, parseArguments(argsPattern, EnvironmentSettings::terminalArgsPatternMagicVariables(), false), payloadArgsWithArgv0);
}

ExternalResource::ExternalResource() {
    Q_INIT_RESOURCE(qsynedit_qmake_qmake_qm_files);
    Q_INIT_RESOURCE(redpanda_qt_utils_qmake_qmake_qm_files);
}

ExternalResource::~ExternalResource() {
    Q_CLEANUP_RESOURCE(qsynedit_qmake_qmake_qm_files);
    Q_CLEANUP_RESOURCE(redpanda_qt_utils_qmake_qmake_qm_files);
}

#ifdef Q_OS_WINDOWS
bool applicationHasUtf8Manifest(const wchar_t *path)
{
    auto module = resourcePointer(LoadLibraryExW(path, nullptr, LOAD_LIBRARY_AS_DATAFILE), &FreeLibrary);
    if (!module)
        return false;
    HRSRC resInfo = FindResourceW(
        module.get(),
        MAKEINTRESOURCEW(1) /* CREATEPROCESS_MANIFEST_RESOURCE_ID */,
        MAKEINTRESOURCEW(24) /* RT_MANIFEST */);
    if (!resInfo)
        return false;
    auto res = resourcePointer(LoadResource(module.get(), resInfo), &FreeResource);
    if (!res)
        return false;
    char *data = (char *)LockResource(res.get());
    DWORD size = SizeofResource(module.get(), resInfo);
    QByteArray manifest(data, size);
    QDomDocument doc;
    if (!doc.setContent(manifest))
        return false;
    QDomNodeList acpNodes = doc.elementsByTagName("activeCodePage");
    if (acpNodes.isEmpty())
        return false;
    QDomElement acpNode = acpNodes.item(0).toElement();
    // case sensitive
    // ref. https://devblogs.microsoft.com/oldnewthing/20220531-00/?p=106697
    return acpNode.text() == QStringLiteral("UTF-8");
}

bool osSupportsUtf8Manifest()
{
    // since Windows 10 1903 (accurate build number unknown)
    // ref. https://learn.microsoft.com/en-us/windows/apps/design/globalizing/use-utf8-code-page
    return QOperatingSystemVersion::current().microVersion() >= 18362;
}

bool applicationIsUtf8(const QString &path)
{
    static bool systemIsUtf8 = GetACP() == CP_UTF8;
    if (systemIsUtf8)
        return true;
    if (!fileExists(path))
        return false;
    return osSupportsUtf8Manifest() && applicationHasUtf8Manifest((wchar_t *)path.constData());
}
#endif

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

static const QMap<QString,FileType> FileTypeMapping{
    {"None", FileType::None},
    {"ATTASM", FileType::GAS},
    {"INTELASM", FileType::GAS},
    {"LUA", FileType::LUA},
    {"CSource", FileType::CSource}, // c source file (.c)
    {"CppSource", FileType::CppSource}, // c++ source file (.cpp)
    {"CCppHeader", FileType::CCppHeader}, // c header (.h)
    {"WindowsResourceSource", FileType::WindowsResourceSource}, // resource source (.res)
    {"Project", FileType::Project}, //Red Panda C++ Project (.dev)
    {"Text", FileType::Text}, // text file
    {"FragmentShader", FileType::FragmentShader},
    {"VerticeShader", FileType::VerticeShader},
    {"ModuleDef", FileType::ModuleDef}, // Windows Module Definition
    {"MakeFile", FileType::MakeFile},
    {"Other", FileType::Other},  // Any others
    {"NASM", FileType::NASM},  // Any others
    {"GAS", FileType::GAS},  // Any others
};

QString fileTypeToName(FileType fileType)
{
    for(auto i=FileTypeMapping.constBegin();i!=FileTypeMapping.constEnd();++i) {
        if (i.value()==fileType)
            return i.key();
    }
    return "None";
}

FileType nameToFileType(const QString &name)
{
    return FileTypeMapping.value(name, FileType::None);
}
