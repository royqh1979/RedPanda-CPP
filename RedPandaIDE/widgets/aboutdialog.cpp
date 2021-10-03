#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "../systemconsts.h"

AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->lblTitle->setText(ui->lblTitle->text() + tr("Version: ") + DEVCPP_VERSION);
    ui->lblContent->setText(ui->lblContent->text()
                            .arg(qVersion())
                            .arg("GCC 10.3.0",__DATE__, __TIME__));
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
