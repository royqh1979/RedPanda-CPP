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

Settings* createAppSettings(const QString& filepath = QString()) {
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
            return nullptr;
        }
    }

    if (fileInfo.exists() && !fileInfo.isWritable()) {
        QMessageBox::critical(nullptr, QObject::tr("Error"),
            QString(QObject::tr("Can't write to configuration file %1")).arg(filename));

        return nullptr;
    }
    return new Settings(filename);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    qRegisterMetaType<PCompileIssue>("PCompileIssue");
    qRegisterMetaType<PCompileIssue>("PCompileIssue&");

    try {

        SystemConsts systemConsts;
        pSystemConsts = &systemConsts;

        //load settings
        pSettings = createAppSettings();
        if (pSettings == nullptr) {
            return -1;
        }
        auto settings = std::unique_ptr<Settings>(pSettings);
        settings->environment().load();
        settings->compilerSets().loadSets();
        settings->editor().load();

        //Translation must be loaded after language setting is loaded
        QTranslator trans;
        trans.load("RedPandaIDE_"+pSettings->environment().language(),":/translations");
        app.installTranslator(&trans);

        //Color scheme settings must be loaded after translation
        pColorManager = new ColorManager();

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
