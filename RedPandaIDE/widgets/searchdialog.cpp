#include "searchdialog.h"
#include "ui_searchdialog.h"
#include <QTabBar>

SearchDialog::SearchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SearchDialog)
{
    ui->setupUi(this);
    mTabBar = new QTabBar();
    mTabBar->addTab(tr("Find"));
    mTabBar->addTab(tr("Find in files"));
    mTabBar->addTab(tr("Replace"));
    mTabBar->addTab(tr("Replace in files"));
    ui->dialogLayout->insertWidget(0,mTabBar);
    connect(mTabBar,&QTabBar::currentChanged,this, &SearchDialog::onTabChanged);
}

SearchDialog::~SearchDialog()
{
    delete ui;
}

void SearchDialog::find(const QString &text)
{
    mTabBar->setCurrentIndex(0);
    ui->cbFind->setCurrentText(text);
    show();
}

void SearchDialog::findInFiles(const QString &text)
{
    mTabBar->setCurrentIndex(1);
    ui->cbFind->setCurrentText(text);
    show();
}

void SearchDialog::replace(const QString &sFind, const QString &sReplace)
{
    mTabBar->setCurrentIndex(2);
    ui->cbFind->setCurrentText(sFind);
    ui->cbReplace->setCurrentText(sReplace);
    show();
}

void SearchDialog::replaceInFiles(const QString &sFind, const QString &sReplace)
{
    mTabBar->setCurrentIndex(3);
    ui->cbFind->setCurrentText(sFind);
    ui->cbReplace->setCurrentText(sReplace);
    show();
}

void SearchDialog::onTabChanged()
{
    bool isfind = (mTabBar->currentIndex() == 0);
    bool isfindfiles = (mTabBar->currentIndex() == 1);
    bool isreplace = (mTabBar->currentIndex() == 2);
    bool isreplacefiles = (mTabBar->currentIndex() == 3);

    ui->lblReplace->setVisible(isreplace || isreplacefiles);
    ui->cbReplace->setVisible(isreplace || isreplacefiles);

    ui->grpOrigin->setVisible(isfind || isreplace);
    ui->grpScope->setVisible(isfind || isreplace);
    ui->grpWhere->setVisible(isfindfiles || isreplacefiles);
    ui->grpDirection->setVisible(isfind || isreplace);
    // grpOption is always visible

    // Disable project search option when none is open
//    rbProjectFiles.Enabled := Assigned(MainForm.Project);
    ui->rbProject->setEnabled(false);
//    if not Assigned(MainForm.Project) then
//      rbOpenFiles.Checked := true;

    // Disable prompt when doing finds
    ui->chkPrompt->setEnabled(isreplace or isreplacefiles);

    if (isfind || not isfindfiles) {
        ui->btnExecute->setText(tr("Find"));
    } else {
        ui->btnExecute->setText(tr("Replace"));
    }
    setWindowTitle(mTabBar->tabText(mTabBar->currentIndex()));
}

void SearchDialog::on_cbFind_currentTextChanged(const QString &)
{
    ui->btnExecute->setEnabled(!ui->cbFind->currentText().isEmpty());
}

void SearchDialog::on_btnCancel_clicked()
{
    this->close();
}

void SearchDialog::closeEvent(QCloseEvent *event)
{
    this->deleteLater();
}

void SearchDialog::on_btnExecute_clicked()
{

}
