#include "choosethemedialog.h"
#include "ui_choosethemedialog.h"

ChooseThemeDialog::ChooseThemeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChooseThemeDialog)
{
    ui->setupUi(this);
    ui->rbDark->setChecked(true);
}

ChooseThemeDialog::~ChooseThemeDialog()
{
    delete ui;
}

ChooseThemeDialog::Theme ChooseThemeDialog::theme()
{
    if (ui->rbDark->isChecked())
        return Theme::Dark;
    return Theme::Light;
}

void ChooseThemeDialog::on_btnOk_clicked()
{
    accept();
}

