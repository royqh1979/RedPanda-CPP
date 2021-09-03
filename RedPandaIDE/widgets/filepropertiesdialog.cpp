#include "filepropertiesdialog.h"
#include "ui_filepropertiesdialog.h"

FilePropertiesDialog::FilePropertiesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FilePropertiesDialog)
{
    ui->setupUi(this);
}

FilePropertiesDialog::~FilePropertiesDialog()
{
    delete ui;
}
