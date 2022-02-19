#include "gitbranchdialog.h"
#include "ui_gitbranchdialog.h"
#include "gitmanager.h"

GitBranchDialog::GitBranchDialog(const QString& folder, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GitBranchDialog),
    mFolder(folder)
{
    ui->setupUi(this);
    mManager = new GitManager();
    int current=-1;
    QStringList branches =mManager->listBranches(mFolder,current);
    ui->lstBranches->addItems(branches);
    ui->lstBranches->setCurrentIndex(current);
    ui->rbBranch->setChecked(true);
    ui->rbNonSpecifyTrack->setChecked(true);
    ui->txtNewBranch->setEnabled(false);
    if (branches.isEmpty()) {
        QString currentBranch;
        if (mManager->hasRepository(mFolder,currentBranch)) {
            ui->lstBranches->addItem(currentBranch);
            ui->btnOk->setEnabled(false);
        }
        ui->grpOptions->setEnabled(false);
    }
}

GitBranchDialog::~GitBranchDialog()
{
    delete mManager;
    delete ui;
}

void GitBranchDialog::on_btnCancel_clicked()
{
    reject();
}


void GitBranchDialog::on_btnOk_clicked()
{
    QString branch = ui->lstBranches->currentText();
    QString text;
    if (ui->chkCreate->isChecked())
        text = ui->txtNewBranch->text();
    else
        text = ui->lstBranches->currentText();
    bool result = false;
    if (!text.isEmpty()) {
        result = mManager->switchToBranch(
                    mFolder,
                    ui->txtNewBranch->text(),
                    ui->chkCreate->isChecked(),
                    ui->chkForce->isChecked(),
                    ui->chkMerge->isChecked(),
                    ui->rbForceTrack->isChecked(),
                    ui->rbForceNoTrack->isChecked(),
                    ui->chkForceCreation->isChecked());
    }
    if (result)
        accept();
    else
        reject();
}


void GitBranchDialog::on_lstBranches_currentIndexChanged(int /*index*/)
{
    ui->txtNewBranch->setText("branch_"+ui->lstBranches->currentText());
}


void GitBranchDialog::on_chkCreate_stateChanged(int /*arg1*/)
{
    ui->txtNewBranch->setEnabled(ui->chkCreate->isChecked());
}

void GitBranchDialog::closeEvent(QCloseEvent */* event */)
{
    reject();
}

