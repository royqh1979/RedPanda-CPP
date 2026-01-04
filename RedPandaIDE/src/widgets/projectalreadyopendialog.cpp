#include "projectalreadyopendialog.h"
#include "ui_projectalreadyopendialog.h"

ProjectAlreadyOpenDialog::ProjectAlreadyOpenDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ProjectAlreadyOpenDialog)
{
    ui->setupUi(this);
}

ProjectAlreadyOpenDialog::~ProjectAlreadyOpenDialog()
{
    delete ui;
}

void ProjectAlreadyOpenDialog::on_btnCancel_clicked()
{
    reject();
}

ProjectAlreadyOpenDialog::OpenType ProjectAlreadyOpenDialog::openType() const
{
    return mOpenType;
}

void ProjectAlreadyOpenDialog::setOpenType(OpenType newOpenType)
{
    mOpenType = newOpenType;
}

void ProjectAlreadyOpenDialog::closeEvent(QCloseEvent */*event*/)
{
    reject();
}


void ProjectAlreadyOpenDialog::on_btnThisWindow_clicked()
{
    mOpenType = OpenType::ThisWindow;
    accept();
}


void ProjectAlreadyOpenDialog::on_btnNewWindow_clicked()
{
    mOpenType = OpenType::NewWindow;
    accept();
}

