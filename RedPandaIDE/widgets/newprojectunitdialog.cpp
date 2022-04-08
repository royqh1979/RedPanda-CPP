#include "newprojectunitdialog.h"
#include "ui_newprojectunitdialog.h"
#include "../iconsmanager.h"
#include "../utils.h"

#include <QFileDialog>

NewProjectUnitDialog::NewProjectUnitDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewProjectUnitDialog),
    mSuffix("cpp")
{
    ui->setupUi(this);
    onUpdateIcons();
    connect(pIconsManager,&IconsManager::actionIconsUpdated,
            this, &NewProjectUnitDialog::onUpdateIcons);
}

NewProjectUnitDialog::~NewProjectUnitDialog()
{
    delete ui;
}

QString NewProjectUnitDialog::folder() const
{
    return ui->txtFolder->text();
}

void NewProjectUnitDialog::setFolder(const QString &folderName)
{
    if (folderName!=folder()) {
        ui->txtFolder->setText(folderName);
        QDir dir(folder());
        if (filename().isEmpty() || dir.exists(filename())) {
            //todo change filename
            QString newFileName;
            QString ext;
            if (filename().isEmpty()) {
                ext = mSuffix;
            } else {
                ext = QFileInfo(filename()).suffix();
            }
            do {
                newFileName = tr("untitled")+QString("%1").arg(getNewFileNumber());
                if (!ext.isEmpty())
                    newFileName += "." + ext;
            } while (dir.exists(newFileName));
            setFilename(newFileName);
        }
    }
}

QString NewProjectUnitDialog::filename() const
{
    return ui->txtFilename->text();
}

void NewProjectUnitDialog::setFilename(const QString &filename)
{
    ui->txtFilename->setText(filename);
}

void NewProjectUnitDialog::onUpdateIcons()
{
    pIconsManager->setIcon(ui->btnBrowse, IconsManager::ACTION_FILE_OPEN_FOLDER);
}

void NewProjectUnitDialog::on_btnBrowse_clicked()
{
    QString dir = QFileDialog::getExistingDirectory(
                this,
                tr("Choose directory"),
                folder()
                );
    if (!dir.isEmpty()) {
        setFolder(dir);
    }
}

void NewProjectUnitDialog::on_btnOk_clicked()
{
    accept();
}


void NewProjectUnitDialog::on_btnCancel_clicked()
{
    reject();
}


void NewProjectUnitDialog::on_txtFilename_textChanged(const QString &/*arg1*/)
{
    updateBtnOkStatus();
}

void NewProjectUnitDialog::updateBtnOkStatus()
{
    ui->btnOk->setEnabled(!ui->txtFilename->text().isEmpty()
                          && QFileInfo(ui->txtFolder->text()).isDir());
}

const QString &NewProjectUnitDialog::suffix() const
{
    return mSuffix;
}

void NewProjectUnitDialog::setSuffix(const QString &newSuffix)
{
    mSuffix = newSuffix;
}

void NewProjectUnitDialog::closeEvent(QCloseEvent */*event*/)
{
    reject();
}

void NewProjectUnitDialog::on_txtFolder_textChanged(const QString &/*arg1*/)
{
    updateBtnOkStatus();
}

