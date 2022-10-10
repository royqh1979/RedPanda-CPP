#include "gitlogdialog.h"
#include "ui_gitlogdialog.h"
#include "gitmanager.h"
#include "gitresetdialog.h"

#include <QMenu>

//this is not thread safe, but it's the only solution i can find
static GitLogModel::CommitInfoCacheManager GitLogModel_CacheManager;


GitLogDialog::GitLogDialog(const QString& folder, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GitLogDialog),
    mModel(folder)
{
    ui->setupUi(this);
    QItemSelectionModel* m=ui->tblLogs->selectionModel();
    ui->tblLogs->setModel(&mModel);
    delete m;
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
    PCommitInfoCache infoCache = std::make_shared<CommitInfoCache>();
    GitLogModel_CacheManager.insert(this,infoCache);
}

GitLogModel::~GitLogModel()
{
    GitLogModel_CacheManager.remove(this);
}

int GitLogModel::rowCount(const QModelIndex &/*parent*/) const
{
    return mCount;
}

int GitLogModel::columnCount(const QModelIndex &/*parent*/) const
{
    return 3;
}

QVariant GitLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();
    if (role == Qt::DisplayRole) {
        PGitCommitInfo info = commitInfo(index);

        switch(index.column()) {
        case 0:
            return info->authorDate;
        case 1:
            return info->author;
        case 2:
            return info->title;
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

PGitCommitInfo GitLogModel::commitInfo(const QModelIndex &index) const
{
    if (!index.isValid())
        return PGitCommitInfo();
    int row = index.row();
    PCommitInfoCache infoCache = GitLogModel_CacheManager.value(this);
    PGitCommitInfo commitInfo;
    if (!infoCache->contains(row)) {
        GitManager manager;
        QList<PGitCommitInfo> listCommitInfos =
                manager.log(mFolder,row,50);
        if (listCommitInfos.isEmpty()) {
            return PGitCommitInfo();
        }
        for (int i=0;i<listCommitInfos.count();i++) {
            infoCache->insert(row+i,listCommitInfos[i]);
        }
        commitInfo = listCommitInfos[0];
    } else
        commitInfo = (*infoCache)[row];

    return commitInfo;
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

