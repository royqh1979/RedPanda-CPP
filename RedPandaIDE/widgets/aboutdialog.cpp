#include "aboutdialog.h"
#include "ui_aboutdialog.h"
#include "../systemconsts.h"
#include "../version.h"


AboutDialog::AboutDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AboutDialog)
{
    ui->setupUi(this);
    ui->lblTitle->setText(ui->lblTitle->text() + tr("Version: ") + DEVCPP_VERSION);

#ifdef  __GNUC__
    ui->lblContent->setText(ui->lblContent->text()
                            .arg(qVersion())
                            .arg(QString("GCC %1.%2")
                                 .arg(__GNUC__)
                                 .arg(__GNUC_MINOR__)
                                 ,__DATE__, __TIME__));
#else
    ui->lblContent->setText(ui->lblContent->text()
                            .arg(qVersion())
                            .arg("Non-GCC Compiler"
                                 ,__DATE__, __TIME__));
#endif
}

AboutDialog::~AboutDialog()
{
    delete ui;
}
