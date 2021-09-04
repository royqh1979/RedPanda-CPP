#include "mainwindow.h"
#include "settings.h"
#include "systemconsts.h"
#include "utils.h"
#include <QApplication>
#include <QDir>
#include <QTranslator>
#include <QDebug>
#include <QStandardPaths>
#include <QMessageBox>
#include <QStringList>
#include "common.h"
#include "colorscheme.h"
#include "iconsmanager.h"
#include "autolinkmanager.h"
#include "parser/parserutils.h"

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

        //load settings
        pSettings = new Settings(settingFilename);
        auto settings = std::unique_ptr<Settings>(pSettings);

        //Color scheme settings must be loaded after translation
        pColorManager = new ColorManager();
        pIconsManager = new IconsManager();
        pAutolinkManager = new AutolinkManager();
        pAutolinkManager->load();

        MainWindow mainWindow;
        pMainWindow = &mainWindow;
        mainWindow.show();
        int retCode = app.exec();
        // save settings
        // settings->compilerSets().saveSets();
        return retCode;
    }  catch (BaseError e) {
        QMessageBox::critical(nullptr,QApplication::tr("Error"),e.reason());
        return -1;
    }
}
