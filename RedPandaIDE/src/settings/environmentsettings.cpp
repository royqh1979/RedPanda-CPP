#include "environmentsettings.h"
#include <QLocale>
#include <QDir>
#include "../utils/font.h"
#include "../utils/parsearg.h"
#include "../utils.h"
#include "../systemconsts.h"
#include "dirsettings.h"
#include <QMessageBox>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

EnvironmentSettings::EnvironmentSettings(SettingsPersistor *persistor, DirSettings *dirSettings):
    BaseSettings{persistor, SETTING_ENVIRONMENT},
    mDirSettings{dirSettings}
{

    Q_ASSERT(dirSettings!=nullptr);
}

void EnvironmentSettings::doLoad()
{
    //Appearance
    mTheme = stringValue("theme","dark");
    mInterfaceFont = stringValue("interface_font", defaultUiFont());
    mInterfaceFontSize = intValue("interface_font_size",11);
    mIconZoomFactor = doubleValue("icon_zoom_factor",1.0);
    mLanguage = stringValue("language", QLocale::system().name());
    mIconSet = stringValue("icon_set","contrast");
    mUseCustomIconSet = boolValue("use_custom_icon_set", false);
    mComboboxWheel = boolValue("enable_combobox_wheel", false);

    mCurrentFolder = stringValue("current_folder",QDir::currentPath());
    if (!fileExists(mCurrentFolder)) {
        mCurrentFolder = QDir::currentPath();
    }
    mDefaultOpenFolder = stringValue("default_open_folder",QDir::currentPath());
    if (!fileExists(mDefaultOpenFolder)) {
        mDefaultOpenFolder = QDir::currentPath();
    }

#ifdef Q_OS_WINDOWS
# ifdef WINDOWS_PREFER_OPENCONSOLE
    // prefer UTF-8 compatible OpenConsole.exe
    mUseCustomTerminal = boolValue("use_custom_terminal", true);
# else
    mUseCustomTerminal = boolValue("use_custom_terminal", false);
# endif
#else // UNIX
    mUseCustomTerminal = true;
#endif

    // check saved terminal path
    mTerminalPath = stringValue("terminal_path", "");
    // replace libexec dir for forward compatibility
    mTerminalPath = replacePrefix(mTerminalPath, "%*APP_LIBEXEC_DIR*%", mDirSettings->appLibexecDir());
    mTerminalPath = replacePrefix(mTerminalPath, "%*APP_DIR*%", mDirSettings->appDir());
    mTerminalArgumentsPattern = stringValue("terminal_arguments_pattern", "");

    checkAndSetTerminal();

    mAStylePath = stringValue("astyle_path","");
    if (mAStylePath.isEmpty()
            /* compatibily for old configuration */
        || ( mAStylePath == getFilePath(mDirSettings->appLibexecDir(), "astyle"))
            ) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        QString path = env.value("PATH");
        QStringList pathList = path.split(PATH_SEPARATOR);
        pathList = QStringList{
            mDirSettings->appDir(),
            mDirSettings->appLibexecDir(),
        } + pathList;

        foreach (const QString& folder, pathList) {
            QDir dir{folder};
            QFileInfo fileInfo{dir.absoluteFilePath(ASTYLE_PROGRAM)};
            if (fileInfo.exists()) {
                mAStylePath = fileInfo.absoluteFilePath();
                break;
            }
        }
        mAStylePath = replacePrefix(mAStylePath, mDirSettings->appDir() , "%*APP_DIR*%");
    }

    mHideNonSupportFilesInFileView=boolValue("hide_non_support_files_file_view",true);
    mOpenFilesInSingleInstance = boolValue("open_files_in_single_instance",false);
}

int EnvironmentSettings::interfaceFontSize() const
{
    return mInterfaceFontSize;
}

void EnvironmentSettings::setInterfaceFontSize(int interfaceFontSize)
{
    mInterfaceFontSize = interfaceFontSize;
}

QString EnvironmentSettings::language() const
{
    return mLanguage;
}

void EnvironmentSettings::setLanguage(const QString &language)
{
    mLanguage = language;
}

const QString &EnvironmentSettings::currentFolder() const
{
    return mCurrentFolder;
}

void EnvironmentSettings::setCurrentFolder(const QString &newCurrentFolder)
{
    mCurrentFolder = newCurrentFolder;
}

const QString &EnvironmentSettings::defaultOpenFolder() const
{
    return mDefaultOpenFolder;
}

void EnvironmentSettings::setDefaultOpenFolder(const QString &newDefaultOpenFolder)
{
    mDefaultOpenFolder = newDefaultOpenFolder;
}

const QString &EnvironmentSettings::iconSet() const
{
    return mIconSet;
}

void EnvironmentSettings::setIconSet(const QString &newIconSet)
{
    mIconSet = newIconSet;
}

QString EnvironmentSettings::terminalPath() const
{
    return mTerminalPath;
}

void EnvironmentSettings::setTerminalPath(const QString &terminalPath)
{
    mTerminalPath = terminalPath;
}

QString EnvironmentSettings::AStylePath() const
{
    QString path = mAStylePath;
    if (path.isEmpty())
        path = getFilePath(mDirSettings->appLibexecDir(),ASTYLE_PROGRAM);
    else {
        // replace libexec dir for forward compatibility
        path = replacePrefix(path, "%*APP_LIBEXEC_DIR*%", mDirSettings->appLibexecDir());
        path = replacePrefix(path, "%*APP_DIR*%", mDirSettings->appDir());
    }
    return path;
}

void EnvironmentSettings::setAStylePath(const QString &aStylePath)
{
    mAStylePath = aStylePath;
    mAStylePath = replacePrefix(mAStylePath, mDirSettings->appDir() , "%*APP_DIR*%");
}

QString EnvironmentSettings::terminalArgumentsPattern() const
{
    return mTerminalArgumentsPattern;
}

void EnvironmentSettings::setTerminalArgumentsPattern(const QString &argsPattern)
{
    mTerminalArgumentsPattern = argsPattern;
}

bool EnvironmentSettings::useCustomIconSet() const
{
    return mUseCustomIconSet;
}

void EnvironmentSettings::setUseCustomIconSet(bool newUseCustomIconSet)
{
    mUseCustomIconSet = newUseCustomIconSet;
}

bool EnvironmentSettings::hideNonSupportFilesInFileView() const
{
    return mHideNonSupportFilesInFileView;
}

void EnvironmentSettings::setHideNonSupportFilesInFileView(bool newHideNonSupportFilesInFileView)
{
    mHideNonSupportFilesInFileView = newHideNonSupportFilesInFileView;
}

bool EnvironmentSettings::openFilesInSingleInstance() const
{
    return mOpenFilesInSingleInstance;
}

void EnvironmentSettings::setOpenFilesInSingleInstance(bool newOpenFilesInSingleInstance)
{
    mOpenFilesInSingleInstance = newOpenFilesInSingleInstance;
}

double EnvironmentSettings::iconZoomFactor() const
{
    return mIconZoomFactor;
}

void EnvironmentSettings::setIconZoomFactor(double newIconZoomFactor)
{
    mIconZoomFactor = newIconZoomFactor;
}

QString EnvironmentSettings::queryPredefinedTerminalArgumentsPattern(const QString &executable) const
{
    QString execName = extractFileName(executable);
    foreach (const TerminalItem& item, loadTerminalList()) {
        QString termName = extractFileName(item.terminal);
        if (termName.compare(execName,PATH_SENSITIVITY)==0) return item.param;
    }
    return QString();
}

bool EnvironmentSettings::useCustomTerminal() const
{
    return mUseCustomTerminal;
}

void EnvironmentSettings::setUseCustomTerminal(bool newUseCustomTerminal)
{
    mUseCustomTerminal = newUseCustomTerminal;
}

void EnvironmentSettings::checkAndSetTerminal()
{
    if (isTerminalValid()) return;

    QStringList pathList = getExecutableSearchPaths();
    QList<TerminalItem> terminalList = loadTerminalList();
    for (const TerminalItem& termItem:terminalList) {
        QString term=termItem.terminal;
        term = replacePrefix(term, "%*APP_DIR*%", mDirSettings->appDir());
        QFileInfo info{term};
        QString absoluteTerminalPath;
        if (info.isAbsolute()) {
            absoluteTerminalPath = info.absoluteFilePath();
            if(fileExists(absoluteTerminalPath)) {
                mTerminalPath = absoluteTerminalPath;
                mTerminalArgumentsPattern = termItem.param;
                return;
            }
        } else {
            for (const QString &dirPath: pathList) {
                QDir dir{dirPath};
                absoluteTerminalPath = dir.absoluteFilePath(termItem.terminal);
                if(fileExists(absoluteTerminalPath)) {
                    mTerminalPath = absoluteTerminalPath;
                    mTerminalArgumentsPattern = termItem.param;
                    return;
                }
            }
        }
    }
    //Can't Find a term
    QMessageBox::critical(
                nullptr,
                QCoreApplication::translate("Settings","Error"),
                QCoreApplication::translate("Settings","Can't find terminal program!"));
}

QMap<QString, QString> EnvironmentSettings::terminalArgsPatternMagicVariables()
{
    return mTerminalArgsPatternMagicVariables;
}

bool EnvironmentSettings::comboboxWheel() const
{
    return mComboboxWheel;
}

void EnvironmentSettings::setComboboxWheel(bool newComboboxWheel)
{
    mComboboxWheel=newComboboxWheel;
}

QList<EnvironmentSettings::TerminalItem> EnvironmentSettings::loadTerminalList() const
{
#ifdef Q_OS_WINDOWS
    QString terminalListFilename(":/config/terminal-windows.json");
#else // UNIX
    QString terminalListFilename(":/config/terminal-unix.json");
#endif
    QFile terminalListFile(terminalListFilename);
    if (!terminalListFile.open(QFile::ReadOnly))
        throw FileError(QObject::tr("Can't open file '%1' for read.")
                            .arg(terminalListFilename));
    QByteArray terminalListContent = terminalListFile.readAll();
    QJsonDocument terminalListDocument(QJsonDocument::fromJson(terminalListContent));

    QList<EnvironmentSettings::TerminalItem> result;
    // determing terminal (if not set yet) and build predefined arguments pattern map from our list
    foreach (const auto &terminalGroup, terminalListDocument.array()) {
        const QJsonArray &terminals = terminalGroup.toObject()["terminals"].toArray();
        foreach (const auto &terminal_, terminals) {
            const QJsonObject& terminal = terminal_.toObject();
            QString path = terminal["path"].toString();
            QString termExecutable = QFileInfo(path).fileName();
            QString pattern = terminal["argsPattern"].toString();
            EnvironmentSettings::TerminalItem terminalItem;
            path = replacePrefix(path, "%*APP_DIR*%", mDirSettings->appDir());
            terminalItem.terminal = path;
            terminalItem.param = pattern;
            result.append(terminalItem);
        }
    }
    return result;
}

bool EnvironmentSettings::isTerminalValid()
{
    // don't use custom terminal
    if (!mUseCustomTerminal) return true;
    // terminal patter is empty
    if (mTerminalArgumentsPattern.isEmpty()) return false;

    QStringList patternItems = parseArguments(mTerminalArgumentsPattern, mTerminalArgsPatternMagicVariables, false);

    if (!(patternItems.contains("$argv")
          || patternItems.contains("$command")
          || patternItems.contains("$tmpfile"))) {
        // program not referenced
        return false;
    }
    QFileInfo termPathInfo{mTerminalPath};
    if (termPathInfo.isAbsolute()) {
        return termPathInfo.exists();
    } else {
        QStringList pathList = getExecutableSearchPaths();
        for (const QString &dirName: pathList) {
            QDir dir{dirName};
            QString absoluteTerminalPath = dir.absoluteFilePath(mTerminalPath);
            QFileInfo absTermPathInfo(absoluteTerminalPath);
            if (absTermPathInfo.exists()) return true;
        }
    }
    return false;
}

void EnvironmentSettings::doSave()
{
    //Appearance
    saveValue("theme", mTheme);
    saveValue("interface_font", mInterfaceFont);
    saveValue("interface_font_size", mInterfaceFontSize);
    saveValue("icon_zoom_factor",mIconZoomFactor);
    saveValue("language", mLanguage);
    saveValue("icon_set",mIconSet);
    saveValue("use_custom_icon_set", mUseCustomIconSet);
    saveValue("use_custom_theme", mUseCustomTheme);
    saveValue("enable_combobox_wheel", mComboboxWheel);

    saveValue("current_folder",mCurrentFolder);
    saveValue("default_open_folder",mDefaultOpenFolder);
    QString terminalPath = mTerminalPath;
    if (isGreenEdition())
    {
        // APP_DIR trick for windows portable app
        // For non-portable app (other platform or Windows installer), multiple instances
        // share the same configuration and thus the trick may break terminal path
        terminalPath = replacePrefix(terminalPath, mDirSettings->appDir(), "%*APP_DIR*%");
    }

    saveValue("terminal_path",terminalPath);
    saveValue("terminal_arguments_pattern",mTerminalArgumentsPattern);
#ifdef Q_OS_WINDOWS
    saveValue("use_custom_terminal",mUseCustomTerminal);
#endif
    saveValue("astyle_path",mAStylePath);

    saveValue("hide_non_support_files_file_view",mHideNonSupportFilesInFileView);
    saveValue("open_files_in_single_instance",mOpenFilesInSingleInstance);
}

QString EnvironmentSettings::interfaceFont() const
{
    return mInterfaceFont;
}

void EnvironmentSettings::setInterfaceFont(const QString &interfaceFont)
{
    mInterfaceFont = interfaceFont;
}

QString EnvironmentSettings::theme() const
{
    return mTheme;
}

void EnvironmentSettings::setTheme(const QString &theme)
{
    mTheme = theme;
}

const QMap<QString, QString> EnvironmentSettings::mTerminalArgsPatternMagicVariables = {
    {"term", "$term"},
    {"integrated_term", "$integrated_term"},
    {"argv", "$argv"},
    {"command", "$command"},
    {"unix_command", "$unix_command"},
    {"dos_command", "$dos_command"},
    {"lpCommandLine", "$lpCommandLine"},
    {"tmpfile", "$tmpfile"},
    {"sequential_app_id", "$sequential_app_id"},
};
