#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "editorlist.h"
#include "editor.h"
#include "systemconsts.h"
#include "settings.h"

#include <QCloseEvent>
#include <QFileDialog>
#include <QLabel>

MainWindow* pMainWindow;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    // status bar
    mFileInfoStatus=new QLabel(this);
    mFileEncodingStatus = new QLabel(this);
    mFileInfoStatus->setStyleSheet("margin-left:10px; margin-right:10px");
    mFileEncodingStatus->setStyleSheet("margin-left:10px; margin-right:10px");
    ui->statusbar->addWidget(mFileInfoStatus);
    ui->statusbar->addWidget(mFileEncodingStatus);
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

void MainWindow::updateStatusBarForEncoding() {
    Editor * editor = mEditorList->getEditor();
    if (editor!=NULL) {
        mFileEncodingStatus->setText(editor->fileEncoding());
    }
}

void MainWindow::updateStatusBarForEditingInfo(int line,int col,int lines,int charCount)
{
    Editor * editor = mEditorList->getEditor();
    if (editor!=NULL) {
        mFileInfoStatus->setText(
            QString(tr("Line: %1  Col: %2  Lines: %3 Chars: %4")).arg(line)
                    .arg(col).arg(lines).arg(charCount));
    }
}

void MainWindow::openFiles(const QStringList &files)
{
    mEditorList->beginUpdate();
    auto end = finally([this] {
        this->mEditorList->endUpdate();
    });
    for (QString file:files) {
        openFile(file);
    }
    mEditorList->endUpdate();
}

void MainWindow::openFile(const QString &filename)
{
    Editor* editor = mEditorList->findOpenedEditor(filename);
    if (editor!=nullptr) {
        editor->activate();
        return;
    }
    editor = mEditorList->newEditor(filename,ENCODING_AUTO_DETECT,
                                    false,false);
    editor->activate();
    this->updateStatusBarForEncoding();
}

void MainWindow::setupActions() {

}


void MainWindow::on_actionNew_triggered()
{
    Editor * editor=mEditorList->newEditor("",ENCODING_AUTO_DETECT,false,true);
    editor->activate();
    updateStatusBarForEncoding();
}

void MainWindow::on_EditorTabsLeft_tabCloseRequested(int index)
{
    Editor* editor = mEditorList->getEditor(index);
    mEditorList->closeEditor(editor);
}

void MainWindow::on_actionOpen_triggered()
{
    QString selectedFileFilter = pSystemConsts->defaultFileFilter();
    QStringList files = QFileDialog::getOpenFileNames(pMainWindow,
        tr("Open"), QString(), pSystemConsts->defaultFileFilters().join(";;"),
        &selectedFileFilter);
    openFiles(files);
}

void MainWindow::closeEvent(QCloseEvent *event) {
    if (!mEditorList->closeAll(true)) {
        event->ignore();
        return ;
    }

    delete mEditorList;
    event->accept();
    return;
}

void MainWindow::on_actionSave_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL) {
        editor->save();
    }
}

void MainWindow::on_actionSaveAs_triggered()
{
    Editor * editor = mEditorList->getEditor();
    if (editor != NULL) {
        editor->saveAs();
    }
}
