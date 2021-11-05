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
#include "common.h"
#include "colorscheme.h"
#include "iconsmanager.h"
#include "autolinkmanager.h"
#include "platform.h"
#include "parser/parserutils.h"
#include "editorlist.h"
#include <windows.h>

class WindowLogoutEventFilter : public QAbstractNativeEventFilter {

    // QAbstractNativeEventFilter interface
public:
    bool nativeEventFilter(const QByteArray &eventType, void *message, long *result) override;
};

bool WindowLogoutEventFilter::nativeEventFilter(const QByteArray & /*eventType*/, void *message, long *result){
    MSG * pMsg = static_cast<MSG *>(message);
    if (pMsg->message == WM_QUERYENDSESSION) {
        if (pMsg->lParam == 0
                || (pMsg->lParam & ENDSESSION_LOGOFF)) {
            if (!pMainWindow->close()) {
                *result = 0;
            } else {
                *result = 1;
            }
            return true;
        }
    }
    return false;
}

QString getSettingFilename(const QString& filepath = QString()) {
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

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    //Translation must be loaded first
    QTranslator trans;
    QString settingFilename = getSettingFilename();
    if (!isGreenEdition()) {
        QDir::setCurrent(QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation)[0]);
    }
    if (settingFilename.isEmpty())
        return -1;
    {
        QSettings languageSetting(settingFilename,QSettings::IniFormat);
        languageSetting.beginGroup(SETTING_ENVIRONMENT);
        QString language = languageSetting.value("language",QLocale::system().name()).toString();
        trans.load("RedPandaIDE_"+language,":/translations");
        app.installTranslator(&trans);
    }

    qRegisterMetaType<PCompileIssue>("PCompileIssue");
    qRegisterMetaType<PCompileIssue>("PCompileIssue&");
    qRegisterMetaType<QVector<int>>("QVector<int>");

    initParser();

    try {

        SystemConsts systemConsts;
        pSystemConsts = &systemConsts;
        pCharsetInfoManager = new CharsetInfoManager();
        auto charsetInfoManager = std::unique_ptr<CharsetInfoManager>(pCharsetInfoManager);
        //load settings
        pSettings = new Settings(settingFilename);
        auto settings = std::unique_ptr<Settings>(pSettings);

        //Color scheme settings must be loaded after translation
        pColorManager = new ColorManager();
        pIconsManager = new IconsManager();
        pAutolinkManager = new AutolinkManager();
        try {
            pAutolinkManager->load();
        } catch (FileError e) {
            QMessageBox::critical(nullptr,
                                  QObject::tr("Can't load autolink settings"),
                                  e.reason(),
                                  QMessageBox::Ok);
        }

        MainWindow mainWindow;
        pMainWindow = &mainWindow;
        if (app.arguments().count()>1) {
            QStringList filesToOpen = app.arguments();
            filesToOpen.pop_front();
            pMainWindow->openFiles(filesToOpen);
        } else {
            if (pSettings->editor().autoLoadLastFiles())
                pMainWindow->loadLastOpens();
            if (pMainWindow->editorList()->pageCount()==0) {
                pMainWindow->newEditor();
            }
        }
        mainWindow.show();
        WindowLogoutEventFilter filter;
        app.installNativeEventFilter(&filter);
        int retCode = app.exec();
        QString configDir = pSettings->dirs().config();
        // save settings
        // settings->compilerSets().saveSets();
        if (mainWindow.shouldRemoveAllSettings()) {
            settings.release();
            delete pSettings;
            QDir dir(configDir);
            dir.removeRecursively();
        }
        return retCode;
    }  catch (BaseError e) {
        QMessageBox::critical(nullptr,QApplication::tr("Error"),e.reason());
        return -1;
    }
}
