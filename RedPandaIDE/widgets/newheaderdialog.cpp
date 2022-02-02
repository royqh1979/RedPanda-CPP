#include "newheaderdialog.h"
#include "ui_newheaderdialog.h"
#include "../iconsmanager.h"
#include "../settings.h"

#include <QFileDialog>

NewHeaderDialog::NewHeaderDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewHeaderDialog)
{
    setWindowFlag(Qt::WindowContextHelpButtonHint,false);
    ui->setupUi(this);
    resize(pSettings->ui().newHeaderDialogWidth(),pSettings->ui().newHeaderDialogHeight());
    updateIcons();
    ui->txtHeader->setFocus();
}

NewHeaderDialog::~NewHeaderDialog()
{
    delete ui;
}

QString NewHeaderDialog::headerName() const
{
    return ui->txtHeader->text();
}

QString NewHeaderDialog::path() const
{
    return ui->txtPath->text();
}

void NewHeaderDialog::setPath(const QString &location)
{
    ui->txtPath->setText(location);
}

void NewHeaderDialog::updateIcons()
{
    pIconsManager->setIcon(ui->btnBrowse, IconsManager::ACTION_FILE_OPEN_FOLDER);
}

void NewHeaderDialog::closeEvent(QCloseEvent */*event*/)
{
    reject();
}

void NewHeaderDialog::on_btnCreate_clicked()
{
    accept();
}


void NewHeaderDialog::on_btnCancel_clicked()
{
    reject();
}


void NewHeaderDialog::on_btnBrowse_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(
                this,
                tr("Path"),
                ui->txtPath->text());
    ui->txtPath->setText(fileName);
}

