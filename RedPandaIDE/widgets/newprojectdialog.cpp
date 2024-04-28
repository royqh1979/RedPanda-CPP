/*
 * Copyright (C) 2020-2022 Roy Qu (royqh1979@gmail.com)
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */
#include "newprojectdialog.h"
#include "ui_newprojectdialog.h"
#include "settings.h"
#include "systemconsts.h"
#include "../iconsmanager.h"
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QPainter>
#include <QPushButton>

NewProjectDialog::NewProjectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewProjectDialog)
{
    setWindowFlag(Qt::WindowContextHelpButtonHint,false);
    ui->setupUi(this);
    ui->lstTemplates->setItemAlignment(Qt::AlignCenter);
    mTemplatesTabBar = new QTabBar(this);
    mTemplatesTabBar->setExpanding(false);
    ui->verticalLayout->insertWidget(0,mTemplatesTabBar);

    readTemplateDirs();

    int i=0;
    QString projectName;
    QString location;
    location = excludeTrailingPathDelimiter(pSettings->dirs().projectDir());
    while (true) {
        i++;
        projectName = QString("Project%1").arg(i);
        QString tempLocation = includeTrailingPathDelimiter(location)+projectName;
        if (!QDir(tempLocation).exists())
            break;
    }
    ui->txtProjectName->setText(projectName);
    ui->txtLocation->setText(location);
    resize(pSettings->ui().newProjectDialogWidth(),pSettings->ui().newProjectDialogHeight());

    connect(mTemplatesTabBar,
            &QTabBar::currentChanged,
            this,
            &NewProjectDialog::updateView
            );
    connect(ui->txtProjectName,
            &QLineEdit::textChanged,
            this,
            &NewProjectDialog::updateProjectLocation);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    onUpdateIcons();
    connect(pIconsManager,&IconsManager::actionIconsUpdated,
            this, &NewProjectDialog::onUpdateIcons);
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

bool NewProjectDialog::makeDefaultLanguage()
{
    return ui->chkMakeDefaultLanguage->isChecked();
}

void NewProjectDialog::addTemplate(const QString &filename)
{
    if (!QFile(filename).exists())
        return;
    PProjectTemplate t = std::make_shared<ProjectTemplate>();
    t->readTemplateFile(filename);
    Settings::PCompilerSet pSet=pSettings->compilerSets().defaultSet();
    if (pSet) {
#ifdef ENABLE_SDCC
        if (pSet->compilerType()==CompilerType::SDCC) {
            if (t->options().type==ProjectType::MicroController)
                mTemplates.append(t);
        } else
#endif
        {
            if (t->options().type!=ProjectType::MicroController)
                mTemplates.append(t);
        }
    } else
        mTemplates.append(t);
}

void NewProjectDialog::readTemplateDirs()
{
    addTemplate(":/templates/empty.template");
    readTemplateDir(pSettings->dirs().data(Settings::Dirs::DataType::Template));
    readTemplateDir(pSettings->dirs().config(Settings::Dirs::DataType::Template));
    rebuildTabs();
    updateView();
}

void NewProjectDialog::readTemplateDir(const QString& folderPath)
{

    QString templateExt(".");
    templateExt += TEMPLATE_EXT;
    QDir dir(folderPath);
    if (!dir.exists())
        return;
    foreach (const QFileInfo& fileInfo,dir.entryInfoList()) {
        if (fileInfo.isFile()
                && fileInfo.fileName().endsWith(templateExt)) {
            addTemplate(fileInfo.absoluteFilePath());
        } else if (fileInfo.isDir()) {
            QDir subDir(fileInfo.absoluteFilePath());
            if (subDir.exists(TEMPLATE_INFO_FILE)) {
                addTemplate(cleanPath(subDir.absoluteFilePath(TEMPLATE_INFO_FILE)));
            }
        }
    }
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
            QString iconFilename = cleanPath(QFileInfo(t->fileName()).absoluteDir().absoluteFilePath(t->icon()));
            QIcon icon=QIcon(iconFilename);
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
            item->setSizeHint(QSize(font().pixelSize()*6,font().pixelSize()*2+64));
            item->setTextAlignment(Qt::AlignHCenter | Qt::AlignTop);
            item->setData(Qt::ToolTipRole,t->name());
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
        if (t->options().isCpp) {
            ui->rdCProject->setEnabled(false);
            ui->rdCppProject->setChecked(true);
        } else {
            ui->rdCProject->setEnabled(true);
            ui->rdCProject->setChecked(true);
            if (pSettings->editor().defaultFileCpp()) {
                ui->rdCppProject->setChecked(true);
            } else {
                ui->rdCProject->setChecked(true);
            }
        }
        if (t->iconInfo().isEmpty()) {
            ui->panelIconInfo->setVisible(false);
        } else {
            ui->panelIconInfo->setVisible(true);
            ui->lblIconInfo->setText(t->iconInfo());
        }
    } else {
        ui->lblDescription->setText("");
        ui->rdCProject->setChecked(false);
        ui->rdCppProject->setChecked(false);
        ui->chkMakeDefaultLanguage->setChecked(false);
        ui->panelIconInfo->setVisible(false);
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
                tr("Choose directory"),
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

void NewProjectDialog::onUpdateIcons()
{
    pIconsManager->setIcon(ui->btnBrowse, IconsManager::ACTION_FILE_OPEN_FOLDER);
}


void NewProjectDialog::on_btnOk_clicked()
{
    accept();
}


void NewProjectDialog::on_btnCancel_clicked()
{
    reject();
}

