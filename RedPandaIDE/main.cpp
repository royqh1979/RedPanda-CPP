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

Settings* createAppSettings(const QString& filepath = QString()) {
    QString filename;
    if (filepath.isEmpty()) {
        if (isGreenEdition()) {
            filename = QApplication::applicationDirPath() + QDir::separator() +
                    "config" + QDir::separator() + APP_SETTSINGS_FILENAME;
        } else {
            filename = QStandardPaths::standardLocations(QStandardPaths::AppDataLocation)[0]
                + QDir::separator() + APP_SETTSINGS_FILENAME;
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

//load translations
    QTranslator trans;
    trans.load(("RedPandaIDE_zh_CN"));
    app.installTranslator(&trans);

    SystemConsts systemConsts;
    pSystemConsts = &systemConsts;

    Settings::CompilerSet testSet("e:/workspace/contributes/Dev-CPP/MinGW32_GCC92");
    qDebug() << testSet.binDirs();
    qDebug() << testSet.CIncludeDirs();
    qDebug() << testSet.CppIncludeDirs();
    qDebug() << testSet.LibDirs();

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
