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
#include "mainwindow.h"
#include "settings.h"
#include "systemconsts.h"
#include "utils.h"
#include <QApplication>
#include <QDir>
#include <QTranslator>
#include <QStandardPaths>
#include <QMessageBox>
#include <QStringList>
#include <QAbstractNativeEventFilter>
#include <QDir>
#include <QScreen>
#include <QLockFile>
#include <QFontDatabase>
#include <QLibraryInfo>
#include "common.h"
#include "colorscheme.h"
#include "iconsmanager.h"
#include "autolinkmanager.h"
#include <qt_utils/charsetinfo.h>
#include "parser/parserutils.h"
#include "editorlist.h"
#include "widgets/choosethemedialog.h"
#include "thememanager.h"
#include "utils/font.h"
#include "problems/ojproblemset.h"

#ifdef Q_OS_WIN
#include <QTemporaryFile>
#include <windows.h>
#include <psapi.h>
#include <QSharedMemory>
#include <QBuffer>
#include <winuser.h>
#include <QFontDatabase>

#include "widgets/cpudialog.h"
#endif

QString getSettingFilename(const QString& filepath, bool& firstRun);
#ifdef Q_OS_WIN
class WindowLogoutEventFilter : public QAbstractNativeEventFilter {

    // QAbstractNativeEventFilter interface
public:
#if QT_VERSION_MAJOR >= 6
    bool nativeEventFilter(const QByteArray &eventType, void *message, qintptr *result) override;
#else
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;
#endif
};

#ifndef WM_DPICHANGED
# define WM_DPICHANGED 0x02e0
#endif

#define WM_APP_OPEN_FILE (WM_APP + 6736 /* “OPEN” on dial pad */)
static_assert(WM_APP_OPEN_FILE < 0xc000);

HWND prevAppInstance = NULL;
BOOL CALLBACK GetPreviousInstanceCallback(HWND hwnd, LPARAM param){
    BOOL result = TRUE;
    WCHAR buffer[4098];
    HINSTANCE hWindowModule = (HINSTANCE)GetWindowLongPtr(hwnd,GWLP_HINSTANCE);

    if (hWindowModule==0)
        return result;

    DWORD processID;

    // Get the ID of the process that created this window
    GetWindowThreadProcessId(hwnd,&processID);
    if (processID==0)
        return result;

    // Get the process associated with the ID
    HANDLE hWindowProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
    if (hWindowProcess == 0)
        return result;

    // Get its module filename
    if (GetModuleFileNameExW(hWindowProcess, hWindowModule, buffer, sizeof(buffer)) == 0)
        return TRUE;

    CloseHandle(hWindowProcess); // not needed anymore
    WCHAR * compareFilename=(WCHAR*)param;
    QString s1=QString::fromWCharArray(compareFilename);
    QString s2=QString::fromWCharArray(buffer);

    //Is from the "same" application?
    if (QString::compare(s1,s2,PATH_SENSITIVITY)==0) {
        //found, stop EnumWindows loop
        prevAppInstance = hwnd;
        return FALSE;
    }

    return TRUE;
}

HWND getPreviousInstance() {
    WCHAR buffer[4098];
    //ShowMessage('ERROR_ALREADY_EXISTS');
    // Store our own module filename
    if (GetModuleFileNameW(GetModuleHandle(NULL), buffer, sizeof(buffer)) == 0)
        return NULL;

    // If that's the case, walk all top level windows and find the previous instance
    // At this point, the program that created the mutex might not have created its MainForm yet
    if (EnumWindows(GetPreviousInstanceCallback, LPARAM(buffer))==0) {
        return prevAppInstance;
    } else
        return NULL;
}

#if QT_VERSION_MAJOR >= 6
bool WindowLogoutEventFilter::nativeEventFilter(const QByteArray & /*eventType*/, void *message, qintptr *result)
#else
bool WindowLogoutEventFilter::nativeEventFilter(const QByteArray & /*eventType*/, void *message, long *result)
#endif
{
    MSG * pMsg = static_cast<MSG *>(message);
    switch(pMsg->message) {
    case WM_QUERYENDSESSION:
        if (pMsg->lParam == 0
                || (pMsg->lParam & ENDSESSION_LOGOFF)) {
            if (!pMainWindow->close()) {
                *result = 0;
            } else {
                *result = 1;
            }
            return true;
        }
        break;
    case WM_DPICHANGED:{
        if (pMsg->hwnd == (HWND)pMainWindow->winId()) {
            int oldDPI = screenDPI();
            //postEvent takes the owner ship
            QEvent * dpiEvent = new QEvent(DPI_CHANGED_EVENT);
            qApp->postEvent(pMainWindow,dpiEvent);
            setScreenDPI(HIWORD(pMsg->wParam));
            int newDPI = screenDPI();
            pMainWindow->updateDPI(oldDPI,newDPI);
        } else if (pMainWindow->cpuDialog() &&
                   (HWND)pMainWindow->cpuDialog()->winId() == pMsg->hwnd) {
            int newDPI = HIWORD(pMsg->wParam);
            pMainWindow->cpuDialog()->updateDPI(newDPI);
        }
        break;
        }
    case WM_APP_OPEN_FILE: {
        QSharedMemory sharedMemory("RedPandaCpp/openfiles");
        if (sharedMemory.attach()) {
            QBuffer buffer;
            QDataStream in(&buffer);
            QStringList files;
            sharedMemory.lock();
            buffer.setData((char*)sharedMemory.constData(), sharedMemory.size());
            buffer.open(QBuffer::ReadOnly);
            in >> files;
            sharedMemory.unlock();
            if (pMainWindow->isMinimized()) {
                pMainWindow->showNormal();
            }
            pMainWindow->openFiles(files);
            sharedMemory.detach();
        }
        return true;
    }
    }
    return false;
}

bool sendFilesToInstance() {
    HWND prevInstance = getPreviousInstance();
    if (prevInstance != NULL) {
        QSharedMemory sharedMemory("RedPandaCpp/openfiles");
        QBuffer buffer;
        buffer.open(QBuffer::ReadWrite);
        QDataStream out(&buffer);
        QStringList filesToOpen = qApp->arguments();
        filesToOpen.pop_front();
        out<<filesToOpen;
        int size = buffer.size();
        if (sharedMemory.create(size)) {
            sharedMemory.lock();
            char *to = (char*)sharedMemory.data();
            const char *from = buffer.data().data();
            memcpy(to, from, qMin(sharedMemory.size(), size));
            sharedMemory.unlock();
            SendMessage(prevInstance,WM_APP_OPEN_FILE,0,0);
            return true;
        }
    }
    return false;
}
#endif

QString getSettingFilename(const QString& filepath, bool& firstRun) {
    QString filename;
    if (filepath.isEmpty()) {
        if (isGreenEdition()) {
            filename = includeTrailingPathDelimiter(QApplication::applicationDirPath()) +
                    "config/"  + APP_SETTSINGS_FILENAME;
        } else {
            filename =includeTrailingPathDelimiter(QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0])
                 + APP_SETTSINGS_FILENAME;
        }
    } else {
        filename = filepath;
    }

    QFileInfo fileInfo(filename);
    firstRun = !fileInfo.exists();
    QDir dir(fileInfo.absoluteDir());
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            QMessageBox::critical(nullptr, QObject::tr("Error"),
                QString(QObject::tr("Can't create configuration folder %1")).arg(dir.absolutePath()));
            return "";
        }
    }

    if (fileInfo.exists() && !fileInfo.isWritable()) {
        QMessageBox::critical(nullptr, QObject::tr("Error"),
            QString(QObject::tr("Can't write to configuration file %1")).arg(filename));

        return "";
    }
    return filename;
}

void setTheme(const QString& theme) {
    pSettings->environment().setTheme(theme);
    ThemeManager themeManager;
    PAppTheme appTheme = themeManager.theme(theme);
    if (appTheme && !appTheme->defaultColorScheme().isEmpty()) {
        pSettings->editor().setColorScheme(appTheme->defaultColorScheme());
        pSettings->editor().save();
    }
    if (appTheme && !appTheme->defaultIconSet().isEmpty()) {
        pSettings->environment().setIconSet(appTheme->defaultIconSet());
    }
    pSettings->environment().save();

}

int main(int argc, char *argv[])
{
//#ifdef Q_OS_WINDOWS
//    // Make title bar and palette follow system-wide dark mode setting on recent Windows releases.
//    // Use freetype as the fontengine
//    qputenv("QT_QPA_PLATFORM", "windows:darkmode=2:fontengine=freetype");
//#endif

#ifdef Q_OS_MACOS
    // in macOS GUI apps, `/usr/local/bin` is not in PATH by default
    // follow the Unix way by prepending it to `/usr/bin`
    {
        QStringList pathList = getExecutableSearchPaths();
        if (!pathList.contains("/usr/local/bin")) {
            auto idxUsrBin = pathList.indexOf("/usr/bin");
            if (idxUsrBin >= 0)
                pathList.insert(idxUsrBin, "/usr/local/bin");
            else
                pathList.append("/usr/local/bin");
        }
        QString newPath = pathList.join(PATH_SEPARATOR);
        qputenv("PATH", newPath.toUtf8());
    }
#endif

    QApplication app(argc, argv);

    app.setAttribute(Qt::AA_UseHighDpiPixmaps);

    ExternalResource resource;

    QLockFile lockFile(QDir::tempPath()+QDir::separator()+"RedPandaDevCppStartUp.lock");
    {
        bool firstRun;
        QString settingFilename = getSettingFilename(QString(), firstRun);
        bool openInSingleInstance = false;
        if (!settingFilename.isEmpty() && !firstRun) {
            QSettings envSetting(settingFilename,QSettings::IniFormat);
            envSetting.beginGroup(SETTING_ENVIRONMENT);
            openInSingleInstance = envSetting.value("open_files_in_single_instance",false).toBool();
        } else if (!settingFilename.isEmpty() && firstRun)
            openInSingleInstance = false;
        if (app.arguments().contains("-ns")) {
            openInSingleInstance = false;
        } else if (app.arguments().contains("-s"))
            openInSingleInstance = true;
        if (openInSingleInstance) {
            int openCount = 0;
            while (true) {
                if (lockFile.tryLock(100))
                    break;
                openCount++;
                if (openCount>100)
                    break;
            }

            if (app.arguments().length()>=2 && openCount<100) {
#ifdef Q_OS_WIN
                if (sendFilesToInstance()) {
                    lockFile.unlock();
                    return 0;
                }
#endif
            }
        }
    }
    //Translation must be loaded first
    QTranslator trans,transQt,transUtils;
    bool firstRun;
    QString settingFilename = getSettingFilename(QString(), firstRun);
    if (!isGreenEdition()) {
        QDir::setCurrent(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0]);
    }
    if (settingFilename.isEmpty()) {
        lockFile.unlock();
        return -1;
    }
    QString language;
    {
        QSettings languageSetting(settingFilename,QSettings::IniFormat);
        languageSetting.beginGroup(SETTING_ENVIRONMENT);
        language = languageSetting.value("language",QLocale::system().name()).toString();

        if (trans.load("RedPandaIDE_"+language,":/i18n/")) {
            app.installTranslator(&trans);
        }
        if (transUtils.load("qt_utils_"+language,":/i18n/")) {
            app.installTranslator(&transUtils);
        }
        QString translationsPath(QLibraryInfo::location(QLibraryInfo::TranslationsPath));
        if (
            // since Qt 5.15.3, `qt_xx.qm` is a wrapper for `qtbase_xx.qm` and other (unused) `qm`s.
            // first, try loading `qt_xx.qm` from standard location (dynamic build) so that it works on Debian 11 (with Qt 5.15.2),
            transQt.load("qt_" + language, translationsPath) ||
            // and then bundled `qtbase_xx.qm` (static build) for simplicity in qmake project.
            transQt.load("qtbase_" + language, ":/translations")
        ) {
            app.installTranslator(&transQt);
        }
    }
    qRegisterMetaType<POJProblem>("POJProblem");
    qRegisterMetaType<PCompileIssue>("PCompileIssue");
    qRegisterMetaType<PCompileIssue>("PCompileIssue&");
    qRegisterMetaType<QVector<int>>("QVector<int>");
    qRegisterMetaType<QHash<int,QString>>("QHash<int,QString>");

    initParser();

    try {

        SystemConsts systemConsts;
        pSystemConsts = &systemConsts;
        CharsetInfoManager charsetInfoManager(language);
        pCharsetInfoManager=&charsetInfoManager;

        //We must use smarter point here, to manually control it's lifetime:
        // when restore default settings, it must be destoyed before we remove all setting files.
        auto settings = std::make_unique<Settings>(settingFilename);
        //load settings
        pSettings = settings.get();
        if (firstRun) {
            pSettings->compilerSets().findSets();
            pSettings->compilerSets().saveSets();
        }
        pSettings->load();
        if (firstRun) {
            //set theme
            ChooseThemeDialog themeDialog;
            themeDialog.setFont(QFont(defaultUiFont(),11));
            themeDialog.exec();
            switch (themeDialog.theme()) {
            case ChooseThemeDialog::Theme::AutoFollowSystem:
                setTheme("system");
                break;
            case ChooseThemeDialog::Theme::Dark:
                setTheme("dark");
                break;
            case ChooseThemeDialog::Theme::Light:
                setTheme("default");
                break;
            default:
                setTheme("default");
            }

            pSettings->editor().setDefaultFileCpp(themeDialog.language()==ChooseThemeDialog::Language::CPlusPlus);
            pSettings->editor().save();

            //auto detect git in path
#ifdef ENABLE_VCS
            pSettings->vcs().detectGitInPath();
#endif
        }
        //Color scheme settings must be loaded after translation
        ColorManager colorManager;
        pColorManager = &colorManager;
        IconsManager iconsManager;
        pIconsManager = &iconsManager;
        AutolinkManager autolinkManager;
        pAutolinkManager = &autolinkManager;
        try {
            pAutolinkManager->load();
        } catch (FileError e) {
            QMessageBox::critical(nullptr,
                                  QObject::tr("Can't load autolink settings"),
                                  e.reason(),
                                  QMessageBox::Ok);
        }
        // qDebug()<<"Load font";
        QFontDatabase::addApplicationFont(":/fonts/asciicontrol.ttf");

        MainWindow mainWindow;
        pMainWindow = &mainWindow;
        if (mainWindow.screen())
            setScreenDPI(mainWindow.screen()->logicalDotsPerInch());
        mainWindow.show();
        if (app.arguments().count()>1) {
            QStringList filesToOpen = app.arguments();
            filesToOpen.pop_front();
            pMainWindow->openFiles(filesToOpen);
        } else {
            if (pSettings->editor().autoLoadLastFiles())
                pMainWindow->loadLastOpens();
            if (pMainWindow->editorList()->pageCount()==0 && !pMainWindow->project()) {
                pMainWindow->newEditor();
            }
        }

        //reset default open folder
        QDir::setCurrent(pSettings->environment().defaultOpenFolder());

        pMainWindow->setFilesViewRoot(pSettings->environment().currentFolder());

#ifdef Q_OS_WIN
        WindowLogoutEventFilter filter;
        app.installNativeEventFilter(&filter);
#endif
        if (lockFile.isLocked()) {
            lockFile.unlock();
        }

        int retCode = app.exec();
        if (mainWindow.shouldRemoveAllSettings()) {
            QString configDir = pSettings->dirs().config();
            settings.release();
            delete pSettings;
            QDir dir(configDir);
            dir.removeRecursively();
        }
        return retCode;
    }  catch (BaseError e) {
        lockFile.unlock();
        QMessageBox::critical(nullptr,QApplication::tr("Error"),e.reason());
        return -1;
    }
}
