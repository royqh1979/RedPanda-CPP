#include "gitlogdialog.h"
#include "ui_gitlogdialog.h"
#include "gitmanager.h"
#include "gitresetdialog.h"

#include <QMenu>

GitLogDialog::GitLogDialog(const QString& folder, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GitLogDialog),
    mModel(folder)
{
    ui->setupUi(this);
    ui->tblLogs->setModel(&mModel);
    ui->tblLogs->setContextMenuPolicy(Qt::ContextMenuPolicy::CustomContextMenu);
    connect(ui->tblLogs,&QTableView::customContextMenuRequested,
            this, &GitLogDialog::onLogsContextMenu);
}

GitLogDialog::~GitLogDialog()
{
    delete ui;
}

GitLogModel::GitLogModel(const QString &folder, QObject *parent):
    QAbstractTableModel(parent),mFolder(folder)
{
    GitManager manager;
    mCount = manager.logCounts(folder);
}

int GitLogModel::rowCount(const QModelIndex &parent) const
{
    return mCount;
}

int GitLogModel::columnCount(const QModelIndex &parent) const
{
    return 3;
}

QVariant GitLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole) {
        int row = index.row();
        GitManager manager;
        QList<PGitCommitInfo> listCommitInfos =
                manager.log(mFolder,row,1);
        if (listCommitInfos.isEmpty()) {
            return QVariant();
        }
        switch(index.column()) {
        case 0:
            return listCommitInfos[0]->authorDate;
        case 1:
            return listCommitInfos[0]->author;
        case 2:
            return listCommitInfos[0]->title;
        }
    }
    return QVariant();
}

QVariant GitLogModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation==Qt::Horizontal && role == Qt::DisplayRole) {
        switch(section) {
        case 0:
            return tr("Date");
        case 1:
            return tr("Author");
        case 2:
            return tr("Title");
        }
    }
    return QVariant();
}

PGitCommitInfo GitLogModel::commitInfo(const QModelIndex &index)
{
    if (!index.isValid())
        return PGitCommitInfo();
    int row = index.row();
    GitManager manager;
    QList<PGitCommitInfo> listCommitInfos =
            manager.log(mFolder,row,1);
    if (listCommitInfos.isEmpty()) {
        return PGitCommitInfo();
    }
    return listCommitInfos[0];
}

const QString &GitLogModel::folder() const
{
    return mFolder;
}

void GitLogDialog::on_btnClose_clicked()
{
    reject();
}

void GitLogDialog::onLogsContextMenu(const QPoint &pos)
{
    QMenu menu(this);
    QString branch;
    GitManager manager;
    if (!manager.hasRepository(mModel.folder(), branch))
        return;
//    menu.addAction(ui->actionRevert);
    menu.addAction(ui->actionReset);
//    menu.addAction(ui->actionBranch);
//    menu.addAction(ui->actionTag);
    ui->actionReset->setText(tr("Reset \"%1\" to this...").arg(branch));
    ui->actionRevert->setText(tr("Revert \"%1\" to this...").arg(branch));
    ui->actionBranch->setText(tr("Create Branch at this version..."));
    ui->actionTag->setText(tr("Create Tag at this version..."));
    menu.exec(ui->tblLogs->mapToGlobal(pos));
}


void GitLogDialog::on_actionReset_triggered()
{
    QModelIndex index = ui->tblLogs->currentIndex();
    if (!index.isValid())
        return;
    PGitCommitInfo commitInfo = mModel.commitInfo(index);
    GitResetDialog resetDialog(mModel.folder());
    if (resetDialog.resetToCommit(commitInfo->commitHash)==QDialog::Accepted)
        accept();
}

