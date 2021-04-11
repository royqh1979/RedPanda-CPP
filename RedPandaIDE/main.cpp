#include "mainwindow.h"
#include "settings.h"
#include "systemconsts.h"
#include <QApplication>
#include <QDir>
#include <QTranslator>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QTranslator trans;
    qDebug()<<QDir::currentPath();
    trans.load(("RedPandaIDE_zh_CN"));
    app.installTranslator(&trans);
    SystemConsts systemConsts;
    pSystemConsts = &systemConsts;
    Settings settings;
    pSettings = &settings;
    MainWindow mainWindow;
    pMainWindow = &mainWindow;
    mainWindow.show();
    return app.exec();
}
