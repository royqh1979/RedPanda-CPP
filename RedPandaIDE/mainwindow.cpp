#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editorlist.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    mEditorList = new EditorList(ui->EditorTabsLeft,
                                 ui->EditorTabsRight,
                                 ui->EditorPanelSplitter,
                                 ui->EditorPanel);
    setupActions();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupActions() {

}


void MainWindow::on_actionNew_triggered()
{
    mEditorList->NewEditor("",etAuto,false,true);
}
