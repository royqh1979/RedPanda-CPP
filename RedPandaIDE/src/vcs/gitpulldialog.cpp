#include "gitpulldialog.h"
#include "ui_gitpulldialog.h"

GitPullDialog::GitPullDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GitPullDialog)
{
    ui->setupUi(this);
}

GitPullDialog::~GitPullDialog()
{
    delete ui;
}
