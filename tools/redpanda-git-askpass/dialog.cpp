#include "dialog.h"
#include "ui_dialog.h"

Dialog::Dialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::Dialog)
{
    ui->setupUi(this);
}

Dialog::~Dialog()
{
    delete ui;
}

int Dialog::showPrompt(const QString &prompt)
{
    ui->txtPrompt->setText(prompt);
    return exec();
}

QString Dialog::getInput()
{
    return ui->txtInput->text();
}


void Dialog::on_txtInput_returnPressed()
{
    if (!ui->txtInput->text().isEmpty())
        accept();
}

void Dialog::closeEvent(QCloseEvent *event)
{
    reject();
}
