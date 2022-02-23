#include "gitremotedialog.h"
#include "ui_gitremotedialog.h"
#include "gitmanager.h"
#include "../iconsmanager.h"
#include "../widgets/infomessagebox.h"

GitRemoteDialog::GitRemoteDialog(const QString& folder, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GitRemoteDialog),
    mFolder(folder)
{
    ui->setupUi(this);
    GitManager manager;
    ui->lstRemotes->addItems(manager.listRemotes(folder));
    connect(pIconsManager, &IconsManager::actionIconsUpdated,
            this, &GitRemoteDialog::updateIcons);
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

void GitRemoteDialog::updateIcons()
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

void GitRemoteDialog::on_btnAdd_clicked()
{
    ui->grpDetail->setEnabled(true);
    ui->pnlProcess->setVisible(true);
    ui->btnProcess->setText(tr("Add"));
    ui->btnRemove->setEnabled(false);
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
    ui->lstRemotes->clear();
    GitManager manager;
    ui->lstRemotes->addItems(manager.listRemotes(mFolder));
    ui->btnRemove->setEnabled(false);
    ui->pnlProcess->setVisible(false);
    ui->grpDetail->setEnabled(false);
}

