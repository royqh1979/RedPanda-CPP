#include "mainwindow.h"
#include "settings.h"
#include "systemconsts.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    SystemConsts systemConsts;
    pSystemConsts = &systemConsts;
    Settings settings;
    pSettings = &settings;
    MainWindow mainWindow;
    pMainWindow = &mainWindow;
    mainWindow.show();
    return app.exec();
}
