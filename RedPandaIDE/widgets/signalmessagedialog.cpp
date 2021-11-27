#include "signalmessagedialog.h"
#include "ui_signalmessagedialog.h"

SignalMessageDialog::SignalMessageDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SignalMessageDialog)
{
    ui->setupUi(this);
}

SignalMessageDialog::~SignalMessageDialog()
{
    delete ui;
}

void SignalMessageDialog::setMessage(const QString &message)
{
    ui->lblMessage->setText(message);
}

bool SignalMessageDialog::openCPUInfo()
{
    return ui->chkOpenCPUInfo->isChecked();
}

void SignalMessageDialog::setOpenCPUInfo(bool value)
{
    ui->chkOpenCPUInfo->setChecked(value);
}
