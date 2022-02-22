#include "gitresetdialog.h"
#include "ui_gitresetdialog.h"

GitResetDialog::GitResetDialog(const QString& folder, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GitResetDialog),
    mFolder(folder)
{
    ui->setupUi(this);
}

GitResetDialog::~GitResetDialog()
{
    delete ui;
}

int GitResetDialog::resetToCommit(const QString &commit)
{
    ui->rbBranch->setEnabled(false);
    ui->cbBranches->setEnabled(false);
    ui->rbTag->setEnabled(false);
    ui->cbTags->setEnabled(false);
    ui->rbCommit->setChecked(true);
    ui->cbCommits->addItem(commit);
    ui->rbMixed->setChecked(true);
    return exec();
}

void GitResetDialog::on_btnOk_clicked()
{
    accept();
}


void GitResetDialog::on_btnCancel_clicked()
{
    reject();
}

