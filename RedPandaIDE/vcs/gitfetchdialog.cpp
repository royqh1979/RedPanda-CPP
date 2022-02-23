#include "gitfetchdialog.h"
#include "ui_gitfetchdialog.h"

GitFetchDialog::GitFetchDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GitFetchDialog)
{
    ui->setupUi(this);
}

GitFetchDialog::~GitFetchDialog()
{
    delete ui;
}
