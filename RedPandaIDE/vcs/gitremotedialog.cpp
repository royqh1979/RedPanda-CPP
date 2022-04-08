#include "gitremotedialog.h"
#include "ui_gitremotedialog.h"
#include "gitmanager.h"
#include "../iconsmanager.h"
#include "../widgets/infomessagebox.h"

GitRemoteDialog::GitRemoteDialog(const QString& folder, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GitRemoteDialog),
    mFolder(folder),
    mChooseMode(false)
{
    ui->setupUi(this);
    GitManager manager;
    mRemotes = manager.listRemotes(folder);
    ui->lstRemotes->addItems(mRemotes);
    connect(pIconsManager, &IconsManager::actionIconsUpdated,
            this, &GitRemoteDialog::onUpdateIcons);
    ui->btnRemove->setEnabled(false);
    ui->pnlProcess->setVisible(false);
    ui->grpDetail->setEnabled(false);
    connect(ui->lstRemotes->selectionModel(),
            &QItemSelectionModel::selectionChanged,
            this,
            &GitRemoteDialog::onRemotesSelectionChanged);
}

GitRemoteDialog::~GitRemoteDialog()
{
    delete ui;
}

QString GitRemoteDialog::chooseRemote()
{
    mChooseMode = true;
    ui->btnClose->setText(tr("Ok"));

    if (exec()==QDialog::Accepted) {
        if (ui->lstRemotes->selectedItems().count()>0)
            return ui->lstRemotes->selectedItems()[0]->text();
    }
    return "";
}

void GitRemoteDialog::onUpdateIcons()
{
    ui->btnAdd->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_ADD));
    ui->btnRemove->setIcon(pIconsManager->getIcon(IconsManager::ACTION_MISC_REMOVE));
}

void GitRemoteDialog::onRemotesSelectionChanged()
{
    bool enabled=(ui->lstRemotes->selectedItems().count()>0);
    ui->btnRemove->setEnabled(enabled);
    ui->pnlProcess->setVisible(enabled);
    ui->grpDetail->setEnabled(enabled);
    if (enabled) {
        QString remoteName = ui->lstRemotes->selectedItems()[0]->text();
        GitManager manager;
        QString remoteURL = manager.getRemoteURL(mFolder,remoteName);
        ui->txtName->setText(remoteName);
        ui->txtURL->setText(remoteURL);
        ui->btnProcess->setText(tr("Update"));
    }
}

void GitRemoteDialog::checkDetails()
{
    if (ui->txtURL->text().isEmpty()) {
        ui->btnProcess->setEnabled(false);
        return;
    }

    if (ui->txtName->text().isEmpty()) {
        ui->btnProcess->setEnabled(false);
        return;
    }

    if (ui->btnProcess->text() == tr("Add")) {
        ui->btnProcess->setEnabled(!mRemotes.contains(ui->txtName->text()));
    } else {
        if (ui->lstRemotes->selectedItems().count()>0) {
            QString remoteName = ui->lstRemotes->selectedItems()[0]->text();
            ui->btnProcess->setEnabled(ui->txtName->text()==remoteName
                                       || !mRemotes.contains(ui->txtName->text()) );
        } else
            ui->btnProcess->setEnabled(false);
    }
}

void GitRemoteDialog::on_btnAdd_clicked()
{
    ui->grpDetail->setEnabled(true);
    ui->pnlProcess->setVisible(true);
    ui->btnProcess->setText(tr("Add"));
    ui->btnRemove->setEnabled(false);
    if (ui->lstRemotes->count()==0) {
        ui->txtName->setText("origin");
        ui->txtURL->setFocus();
    } else
        ui->txtName->setFocus();
}

void GitRemoteDialog::on_btnRemove_clicked()
{
    if (ui->lstRemotes->selectedItems().count()>0) {
        QString remoteName = ui->lstRemotes->selectedItems()[0]->text();
        GitManager manager;
        QString output;
        if (!manager.removeRemote(mFolder,remoteName,output)) {
            InfoMessageBox infoBox;
            infoBox.showMessage(output);
        } else {
            refresh();
        }
    }
}

void GitRemoteDialog::refresh()
{
    ui->txtName->setText("");
    ui->txtURL->setText("");
    ui->lstRemotes->clear();
    GitManager manager;
    mRemotes = manager.listRemotes(mFolder);
    ui->lstRemotes->addItems(mRemotes);
    ui->btnRemove->setEnabled(false);
    ui->pnlProcess->setVisible(false);
    ui->grpDetail->setEnabled(false);
}


void GitRemoteDialog::on_btnProcess_clicked()
{
    if (ui->btnProcess->text() == tr("Add")) {
        // add remote
        QString remoteName = ui->txtName->text();
        QString remoteURL = ui->txtURL->text();
        GitManager manager;
        QString output;
        if (!manager.addRemote(mFolder, remoteName, remoteURL, output)) {
            InfoMessageBox infoBox;
            infoBox.showMessage(output);
        } else {
            refresh();
        }
    } else {
        // update remote
        if (ui->lstRemotes->selectedItems().count()<=0)
            return;
        QString oldName = ui->lstRemotes->selectedItems()[0]->text();
        QString newName = ui->txtName->text();
        QString url = ui->txtURL->text();
        GitManager manager;
        QString output;
        if (!manager.setRemoteURL(mFolder,oldName,url,output)) {
            InfoMessageBox infoBox;
            infoBox.showMessage(output);
            return;
        }
        if (oldName != newName) {
            if (!manager.setRemoteURL(mFolder,oldName,url,output)) {
                InfoMessageBox infoBox;
                infoBox.showMessage(output);
                return;
            }
        }
        refresh();
    }
}


void GitRemoteDialog::on_txtName_textChanged(const QString &/*arg1*/)
{
    checkDetails();
}


void GitRemoteDialog::on_txtURL_textChanged(const QString & /*arg1*/)
{
    checkDetails();
}


void GitRemoteDialog::on_btnClose_clicked()
{
    accept();
}

