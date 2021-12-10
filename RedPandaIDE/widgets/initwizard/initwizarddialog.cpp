#include "initwizarddialog.h"
#include "ui_initwizarddialog.h"

InitWizardDialog::InitWizardDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InitWizardDialog)
{
    ui->setupUi(this);
}

InitWizardDialog::~InitWizardDialog()
{
    delete ui;
}
