#include "newheaderdialog.h"
#include "ui_newheaderdialog.h"

NewHeaderDialog::NewHeaderDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewHeaderDialog)
{
    ui->setupUi(this);
}

NewHeaderDialog::~NewHeaderDialog()
{
    delete ui;
}
