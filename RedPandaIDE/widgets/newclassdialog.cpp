#include "newclassdialog.h"
#include "ui_newclassdialog.h"
#include "../iconsmanager.h"
#include "../settings.h"
#include <QFileDialog>

NewClassDialog::NewClassDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewClassDialog)
{
    setWindowFlag(Qt::WindowContextHelpButtonHint,false);
    ui->setupUi(this);
    resize(pSettings->ui().newClassDialogWidth(),pSettings->ui().newClassDialogHeight());
    updateIcons();
    ui->txtClassName->setFocus();
}

NewClassDialog::~NewClassDialog()
{
}

QString NewClassDialog::className() const
{
    return ui->txtClassName->text();
}

QString NewClassDialog::baseClass() const
{
    return ui->cbBaseClass->currentText();
}

QString NewClassDialog::headerName() const
{
    return ui->txtHeaderName->text();
}

QString NewClassDialog::sourceName() const
{
    return ui->txtSourceName->text();
}

QString NewClassDialog::path() const
{
    return ui->txtPath->text();
}

void NewClassDialog::setPath(const QString &location)
{
    ui->txtPath->setText(location);
}

void NewClassDialog::on_btnCancel_clicked()
{
    this->reject();
}

void NewClassDialog::closeEvent(QCloseEvent *event)
{
    this->reject();
}


void NewClassDialog::on_btnCreate_clicked()
{
    this->accept();
}

void NewClassDialog::updateIcons()
{
    pIconsManager->setIcon(ui->btnBrowsePath, IconsManager::ACTION_FILE_OPEN_FOLDER);
}


void NewClassDialog::on_btnBrowsePath_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(
                this,
                tr("Path"),
                ui->txtPath->text());
    ui->txtPath->setText(fileName);
}


void NewClassDialog::on_txtClassName_textChanged(const QString &/* arg1 */)
{
    ui->txtHeaderName->setText(ui->txtClassName->text().toLower()+".h");
    ui->txtSourceName->setText(ui->txtClassName->text().toLower()+".cpp");
}

