#include "newtemplatedialog.h"
#include "ui_newtemplatedialog.h"
#include "../settings.h"
#include "../projecttemplate.h"
#include "../systemconsts.h"

#include <QFile>
#include <QDir>
#include <QFileInfo>

NewTemplateDialog::NewTemplateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewTemplateDialog)
{
    ui->setupUi(this);
    QStringList categories = findCategories();
    ui->cbCategory->addItems(categories);
    updateCreateState();
}

NewTemplateDialog::~NewTemplateDialog()
{
    delete ui;
}

QString NewTemplateDialog::getName() const
{
    return ui->txtName->text();
}

QString NewTemplateDialog::getDescription() const
{
    return ui->txtDescription->toPlainText();
}

QString NewTemplateDialog::getCategory() const
{
    return ui->cbCategory->currentText();
}

QStringList NewTemplateDialog::findCategories()
{
    QSet<QString> categories;
    readTemplateCategory(":/templates/empty.template",categories);
    readTemplateCategoriesInDir(pSettings->dirs().data(Settings::Dirs::DataType::Template),categories);
    readTemplateCategoriesInDir(pSettings->dirs().config(Settings::Dirs::DataType::Template),categories);
    QStringList result;
    foreach(const QString& s, categories)
        result.append(s);
    result.sort();
    return result;
}

void NewTemplateDialog::readTemplateCategory(const QString &filename, QSet<QString> &categories)
{
    if (!QFile(filename).exists())
        return;
    PProjectTemplate t = std::make_shared<ProjectTemplate>();
    t->readTemplateFile(filename);
    if (!t->category().isEmpty())
        categories.insert(t->category());
}

void NewTemplateDialog::readTemplateCategoriesInDir(const QString &folderPath, QSet<QString> &categories)
{
    QString templateExt(".");
    templateExt += TEMPLATE_EXT;
    QDir dir(folderPath);
    if (!dir.exists())
        return;
    foreach (const QFileInfo& fileInfo,dir.entryInfoList()) {
        if (fileInfo.isFile()
                && fileInfo.fileName().endsWith(templateExt)) {
            readTemplateCategory(fileInfo.absoluteFilePath(),categories);
        } else if (fileInfo.isDir()) {
            QDir subDir(fileInfo.absoluteFilePath());
            readTemplateCategory(cleanPath(subDir.absoluteFilePath(TEMPLATE_INFO_FILE)),categories);
        }
    }

}

void NewTemplateDialog::updateCreateState()
{
    ui->btnCreate->setEnabled(
                !ui->txtName->text().isEmpty()
                && !ui->cbCategory->currentText().isEmpty()
                );
}

void NewTemplateDialog::closeEvent(QCloseEvent */*event*/)
{
    reject();
}

void NewTemplateDialog::on_btnCreate_clicked()
{
    accept();
}


void NewTemplateDialog::on_btnCancel_clicked()
{
    reject();
}


void NewTemplateDialog::on_txtName_textChanged(const QString &/*arg1*/)
{
    updateCreateState();
}


void NewTemplateDialog::on_cbCategory_currentTextChanged(const QString &/*arg1*/)
{
    updateCreateState();
}

