#include "mainwindow.h"
#include "settings.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    Settings s;
    pSettings = &s;
    MainWindow w;
    pMainWindow = &w;
    w.show();
    return a.exec();
}
