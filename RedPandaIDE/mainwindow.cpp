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
    ui->EditorTabsRight->setVisible(false);
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

void MainWindow::on_EditorTabsLeft_tabCloseRequested(int index)
{
    Editor* editor = mEditorList->GetEditor(index,ui->EditorTabsLeft);
    mEditorList->CloseEditor(editor);
}
