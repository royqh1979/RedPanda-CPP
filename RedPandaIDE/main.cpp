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

// we have to wrap the following in a function, or it will crash createAppSettings when debug, don't know why
void showConfigCantWriteMsg(const QString& filename) {
    QMessageBox::information(nullptr, QObject::tr("Error"),
        QString(QObject::tr("Can't write to configuration file %1")).arg(filename));
}

Settings* createAppSettings(const QString& filepath = QString()) {
    QString filename("");
    if (filename.isEmpty()) {

//    //    if (isGreenEdition()) {
//    //        name = QApplication::applicationDirPath() + QDir::separator() +
//    //                "config" + QDir::separator() + APP_SETTSINGS_FILENAME;
//    //    } else {
        filename = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0]
            + QDir::separator() + APP_SETTSINGS_FILENAME;
//    //    }
    } else {
        filename = filepath;
    }

    QDir dir = QFileInfo(filename).absoluteDir();
    if (!dir.exists()) {
        if (!dir.mkpath(dir.absolutePath())) {
            QMessageBox::information(nullptr, QObject::tr("Error"),
                QString(QObject::tr("Can't create configuration folder %1")).arg(dir.absolutePath()));
            return nullptr;
        }
    }

    QFileInfo fileInfo(filename);

    if (fileInfo.exists() && !fileInfo.isWritable()) {
        showConfigCantWriteMsg(filename);
        return nullptr;
    }
    return new Settings(filename);
}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

//load translations
    QTranslator trans;
    trans.load(("RedPandaIDE_zh_CN"));
    app.installTranslator(&trans);

    SystemConsts systemConsts;
    pSystemConsts = &systemConsts;

    pSettings = createAppSettings();
    if (pSettings == nullptr) {
        return -1;
    }
    auto settings = std::unique_ptr<Settings>(pSettings);

    MainWindow mainWindow;
    pMainWindow = &mainWindow;
    mainWindow.show();
    return app.exec();
}
