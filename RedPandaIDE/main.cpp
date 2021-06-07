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
            QMessageBox::information(nullptr, QObject::tr("Error"),
                QString(QObject::tr("Can't create configuration folder %1")).arg(dir.absolutePath()));
            return nullptr;
        }
    }

    if (fileInfo.exists() && !fileInfo.isWritable()) {
        QMessageBox::information(nullptr, QObject::tr("Error"),
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

//load translations
    QTranslator trans;
    trans.load(("RedPandaIDE_zh_CN"));
    app.installTranslator(&trans);

    SystemConsts systemConsts;
    pSystemConsts = &systemConsts;

//    Settings::CompilerSet testSet("e:/workspace/contributes/Dev-CPP/MinGW32_GCC92");
//    qDebug() << testSet.binDirs();
//    qDebug() << testSet.CIncludeDirs();
//    qDebug() << testSet.CppIncludeDirs();
//    qDebug() << testSet.LibDirs();

    pSettings = createAppSettings();
    if (pSettings == nullptr) {
        return -1;
    }
    auto settings = std::unique_ptr<Settings>(pSettings);

    //settings->compilerSets().addSets("e:/workspace/contributes/Dev-CPP/MinGW32_GCC92");
//    settings->compilerSets().findSets();
//    settings->compilerSets().saveSets();
    settings->compilerSets().loadSets();
    settings->editor().load();
//    qDebug() << settings->compilerSets().defaultSet()->binDirs();
//    settings->compilerSets().loadSets();
//    qDebug() << settings->compilerSets().defaultSet()->defines();
//    qDebug() << settings->compilerSets().defaultSet()->CCompiler();
//    qDebug()<<settings->compilerSets().size();
//    qDebug()<<settings->compilerSets().list().at(0)->binDirs();

      // load theme
//    QFile cssFile("dracula.css");
//    if (cssFile.open(QFile::ReadOnly)) {
//        QString qss = QLatin1String(cssFile.readAll());
//        app.setStyleSheet(qss);
//    }
    MainWindow mainWindow;
    pMainWindow = &mainWindow;
    mainWindow.show();
    int retCode = app.exec();
    // save settings
//    settings->compilerSets().saveSets();
    return retCode;
}
