#include "gitmergedialog.h"
#include "ui_gitmergedialog.h"
#include "gitmanager.h"

GitMergeDialog::GitMergeDialog(const QString& folder, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GitMergeDialog),
    mFolder(folder)
{
    ui->setupUi(this);
    mManager = new GitManager();
}

GitMergeDialog::~GitMergeDialog()
{
    delete mManager;
    delete ui;
}
