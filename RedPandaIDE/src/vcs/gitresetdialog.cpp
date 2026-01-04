#include "gitresetdialog.h"
#include "ui_gitresetdialog.h"
#include "gitmanager.h"
#include "../widgets/infomessagebox.h"

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
    GitManager manager;
    QString branch;
    if (!manager.hasRepository(mFolder,branch))
        return QDialog::Rejected;
    ui->rbBranch->setEnabled(false);
    ui->cbBranches->setEnabled(false);
    ui->rbTag->setEnabled(false);
    ui->cbTags->setEnabled(false);
    ui->rbCommit->setChecked(true);
    ui->cbCommits->addItem(commit);
    ui->rbMixed->setChecked(true);
    ui->grpTarget->setTitle(tr("Reset current branch \"%1\" to").arg(branch));
    return exec();
}

void GitResetDialog::on_btnOk_clicked()
{
    GitManager manager;
    GitResetStrategy strategy = GitResetStrategy::Mixed;
    if (ui->rbSoft->isChecked())
        strategy = GitResetStrategy::Soft;
    else if (ui->rbHard->isChecked())
        strategy = GitResetStrategy::Hard;
    QString commit;
    if (ui->rbCommit->isChecked())
        commit = ui->cbCommits->currentText();
    else if (ui->rbTag->isChecked())
        commit = ui->cbTags->currentText();
    else if (ui->rbBranch->isChecked())
        commit = ui->cbBranches->currentText();
    if (commit.isEmpty())
        reject();
    QString output;
    bool result = manager.reset(mFolder,commit,strategy,output);
    if (!output.trimmed().isEmpty()) {
        InfoMessageBox infoBox;
        infoBox.showMessage(output);
    }
    if (result)
        accept();
    else
        reject();
}


void GitResetDialog::on_btnCancel_clicked()
{
    reject();
}

