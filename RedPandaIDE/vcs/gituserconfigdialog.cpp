#include "gituserconfigdialog.h"
#include "ui_gituserconfigdialog.h"
#include "gitmanager.h"
#include "../widgets/infomessagebox.h"

GitUserConfigDialog::GitUserConfigDialog(const QString& folder, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GitUserConfigDialog),
    mFolder(folder)
{
    ui->setupUi(this);
    GitManager manager;
    ui->txtUserName->setText(manager.getUserName(folder));
    ui->txtUserEmail->setText(manager.getUserEmail(folder));
    checkInfo();
}

GitUserConfigDialog::~GitUserConfigDialog()
{
    delete ui;
}

void GitUserConfigDialog::checkInfo()
{
    ui->btnOk->setEnabled(!ui->txtUserEmail->text().isEmpty()
                          && !ui->txtUserName->text().isEmpty());
}

void GitUserConfigDialog::on_btnOk_clicked()
{
    GitManager manager;
    QString output;
    if (!manager.setUserName(mFolder, ui->txtUserName->text(),output)) {
        InfoMessageBox infoBox;
        infoBox.showMessage(output);
        reject();
    }
    if (!manager.setUserEmail(mFolder, ui->txtUserEmail->text(),output)) {
        InfoMessageBox infoBox;
        infoBox.showMessage(output);
        reject();
    }
    accept();
}


void GitUserConfigDialog::on_btnCancel_clicked()
{
    reject();
}

void GitUserConfigDialog::closeEvent(QCloseEvent * /*event*/)
{
    reject();
}


void GitUserConfigDialog::on_txtUserName_textChanged(const QString &/*arg1*/)
{
    checkInfo();
}


void GitUserConfigDialog::on_txtUserEmail_textChanged(const QString &/*arg1*/)
{
    checkInfo();
}

