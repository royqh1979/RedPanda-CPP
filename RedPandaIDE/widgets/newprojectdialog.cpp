#include "newprojectdialog.h"
#include "ui_newprojectdialog.h"
#include "settings.h"
#include "systemconsts.h"

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QPushButton>

NewProjectDialog::NewProjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewProjectDialog)
{
    ui->setupUi(this);
    mTemplatesTabBar = new QTabBar(this);
    ui->verticalLayout->insertWidget(0,mTemplatesTabBar);

    readTemplateDir();
    int i=0;
    QString projectName;
    QString location;
    location = excludeTrailingPathDelimiter(pSettings->dirs().projectDir());
    while (true) {
        i++;
        projectName = tr("Project%1").arg(i);
        QString tempLocation = includeTrailingPathDelimiter(location)+projectName;
        if (!QDir(tempLocation).exists())
            break;
    }
    ui->txtProjectName->setText(projectName);
    ui->txtLocation->setText(location);

    connect(mTemplatesTabBar,
            &QTabBar::currentChanged,
            this,
            &NewProjectDialog::updateView
            );
    connect(ui->txtProjectName,
            &QLineEdit::textChanged,
            this,
            &NewProjectDialog::updateProjectLocation);
}

NewProjectDialog::~NewProjectDialog()
{
    delete ui;
}

PProjectTemplate NewProjectDialog::getTemplate()
{
    QListWidgetItem * item = ui->lstTemplates->currentItem();
    if (!item)
        return PProjectTemplate();
    int index = item->data(Qt::UserRole).toInt();
    return mTemplates[index];
}

QString NewProjectDialog::getLocation()
{
    return ui->txtLocation->text();
}

QString NewProjectDialog::getProjectName()
{
    return ui->txtProjectName->text();
}

bool NewProjectDialog::useAsDefaultProjectDir()
{
    return ui->chkAsDefaultLocation->isChecked();
}

bool NewProjectDialog::isCProject()
{
    return ui->rdCProject->isChecked();
}

bool NewProjectDialog::isCppProject()
{
    return ui->rdCppProject->isChecked();
}

bool NewProjectDialog::makeProjectDefault()
{
    return ui->chkMakeDefault->isChecked();
}

void NewProjectDialog::addTemplate(const QString &filename)
{
    if (!QFile(filename).exists())
        return;
    PProjectTemplate t = std::make_shared<ProjectTemplate>();
    t->readTemplateFile(filename);
    mTemplates.append(t);
}

void NewProjectDialog::readTemplateDir()
{
    QString templateExt(".");
    templateExt += TEMPLATE_EXT;
    QDir dir(pSettings->dirs().templateDir());
    foreach (const QFileInfo& fileInfo,dir.entryInfoList()) {
        if (fileInfo.isFile()
                && fileInfo.fileName().endsWith(templateExt)) {
            addTemplate(fileInfo.absoluteFilePath());
        }
    }
    rebuildTabs();
    updateView();
}

void NewProjectDialog::rebuildTabs()
{
    while (mTemplatesTabBar->count()>0) {
        mTemplatesTabBar->removeTab(0);
    }

    mCategories.clear();
    foreach (const PProjectTemplate& t, mTemplates) {
        QString category = t->category();
        if (category.isEmpty())
            category = tr("Default");
        // Add a page for each unique category
        int tabIndex = mCategories.value(category,-1);
        if (tabIndex<0) {
            tabIndex = mTemplatesTabBar->addTab(category);
            mCategories.insert(category,tabIndex);
        }
    }
    mTemplatesTabBar->setCurrentIndex(0);
}

void NewProjectDialog::updateView()
{
    int index = std::max(0,mTemplatesTabBar->currentIndex());
    if (index>=mTemplatesTabBar->count())
        return;
    ui->lstTemplates->clear();
    for (int i=0;i<mTemplates.count();i++) {
        const PProjectTemplate& t = mTemplates[i];
        QString category = t->category();
        if (category.isEmpty())
            category = tr("Default");
        QString tabText = mTemplatesTabBar->tabText(index);
        if (category == tabText) {
            QListWidgetItem * item;
            QString iconFilename = QDir(pSettings->dirs().templateDir()).absoluteFilePath(t->icon());
            QIcon icon(iconFilename);
            if (icon.isNull()) {
                //todo : use an default icon
                item = new QListWidgetItem(
                       QIcon(":/icons/images/associations/template.ico"),
                       t->name());
            } else {
                 item = new QListWidgetItem(
                        icon,
                        t->name());
            }
            item->setData(Qt::UserRole,i);
            ui->lstTemplates->addItem(item);
        }
    }
}

void NewProjectDialog::updateProjectLocation()
{
    QString newLocation = ui->txtLocation->text();

    QListWidgetItem * current = ui->lstTemplates->currentItem();
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                current && !ui->txtProjectName->text().isEmpty()
                );
}

void NewProjectDialog::on_lstTemplates_itemDoubleClicked(QListWidgetItem *item)
{
    if (item)
        accept();
}


void NewProjectDialog::on_lstTemplates_currentItemChanged(QListWidgetItem *current, QListWidgetItem *)
{
    if (current) {
        int index = current->data(Qt::UserRole).toInt();
        PProjectTemplate t = mTemplates[index];
        ui->lblDescription->setText(t->description());
        if (t->options().useGPP) {
            ui->rdCppProject->setChecked(true);
        } else {
            ui->rdCProject->setChecked(true);
        }
    } else {
        ui->lblDescription->setText("");
        ui->rdCProject->setChecked(false);
        ui->rdCppProject->setChecked(false);
        ui->chkMakeDefault->setChecked(false);
    }
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                current && !ui->txtProjectName->text().isEmpty()
                );
}


void NewProjectDialog::on_btnBrowse_clicked()
{
    QString dirPath = ui->txtLocation->text();
    if (!QDir(dirPath).exists()) {
        dirPath = pSettings->dirs().projectDir();
    }
    QString dir = QFileDialog::getExistingDirectory(
                this,
                "Choose directory",
                dirPath
                );
    if (!dir.isEmpty()) {
        ui->txtLocation->setText(dir);
        QListWidgetItem * current = ui->lstTemplates->currentItem();
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                    current && !ui->txtProjectName->text().isEmpty()
                    );
    }
}

