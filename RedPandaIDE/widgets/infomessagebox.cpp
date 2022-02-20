#include "infomessagebox.h"
#include "ui_infomessagebox.h"

InfoMessageBox::InfoMessageBox(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InfoMessageBox)
{
    ui->setupUi(this);
}

void InfoMessageBox::setMessage(const QString message)
{
    ui->txtMessage->setText(message);
}

InfoMessageBox::~InfoMessageBox()
{
    delete ui;
}

void InfoMessageBox::on_btnOk_clicked()
{
    accept();
}

