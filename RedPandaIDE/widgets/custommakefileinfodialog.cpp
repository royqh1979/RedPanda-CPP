#include "custommakefileinfodialog.h"
#include "ui_custommakefileinfodialog.h"

CustomMakefileInfoDialog::CustomMakefileInfoDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CustomMakefileInfoDialog)
{
    ui->setupUi(this);
}

CustomMakefileInfoDialog::~CustomMakefileInfoDialog()
{
    delete ui;
}

void CustomMakefileInfoDialog::on_pushButton_clicked()
{
    hide();
}

