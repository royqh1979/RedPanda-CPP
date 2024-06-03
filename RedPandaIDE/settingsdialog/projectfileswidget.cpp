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
#include "projectfileswidget.h"
#include "ui_projectfileswidget.h"
#include "../mainwindow.h"
#include "../systemconsts.h"
#include <qt_utils/charsetinfo.h>

ProjectFilesWidget::ProjectFilesWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::ProjectFilesWidget)
{
    ui->setupUi(this);
}

ProjectFilesWidget::~ProjectFilesWidget()
{
    delete ui;
}

void ProjectFilesWidget::doLoad()
{
    if (ui->cbEncoding->count()>0) {
        if (pMainWindow->project()->options().encoding==ENCODING_SYSTEM_DEFAULT) {
            ui->cbEncoding->setItemText(0,tr("Project(%1)").arg(tr("System Default")));
        } else {
            ui->cbEncoding->setItemText(0,tr("Project(%1)").arg(QString(pMainWindow->project()->options().encoding)));
        }
    }
    ui->treeProject->expandAll();
    ui->grpFileOptions->setEnabled(false);
}

void ProjectFilesWidget::doSave()
{
    for (int i=0;i<mUnits.count();i++) {
        PProjectUnit unitCopy = mUnits[i];
        PProjectUnit unit = pMainWindow->project()->findUnit(unitCopy->fileName());
        unit->setPriority(unitCopy->priority());
        unit->setCompile(unitCopy->compile());
        unit->setLink(unitCopy->link());
        unit->setCompileCpp(unitCopy->compileCpp());
        unit->setOverrideBuildCmd(unitCopy->overrideBuildCmd());
        unit->setBuildCmd(unitCopy->buildCmd());
        unit->setEncoding(unitCopy->encoding());
    }
    pMainWindow->project()->saveUnits();
    copyUnits();
    ui->treeProject->expandAll();
    ui->treeProject->clicked(ui->treeProject->currentIndex());
}

void ProjectFilesWidget::onLoaded()
{
    disconnectAbstractItemView(ui->treeProject);
}

PProjectUnit ProjectFilesWidget::currentUnit()
{
    QModelIndex index = ui->treeProject->currentIndex();
    if (!index.isValid())
        return PProjectUnit();
    ProjectModelNode* node = static_cast<ProjectModelNode*>(index.internalPointer());
    if (!node)
        return PProjectUnit();
    if (!node->isUnit)
        return PProjectUnit();
    PProjectUnit unit=node->pUnit.lock();
    if (unit) {
        foreach (PProjectUnit tmpUnit, mUnits) {
            if (tmpUnit->fileName() == unit->fileName())
                return tmpUnit;
        }
    }
    return PProjectUnit();
}

void ProjectFilesWidget::copyUnits()
{
    std::shared_ptr<Project> project = pMainWindow->project();
    if (!project)
        return;
    mUnits.clear();
    foreach (const PProjectUnit& unit, project->unitList()) {
        PProjectUnit unitCopy = std::make_shared<ProjectUnit>(project.get());
        unitCopy->setPriority(unit->priority());
        unitCopy->setCompile(unit->compile());
        unitCopy->setLink(unit->link());
        unitCopy->setCompileCpp(unit->compileCpp());
        unitCopy->setOverrideBuildCmd(unit->overrideBuildCmd());
        unitCopy->setBuildCmd(unit->buildCmd());
        unitCopy->setEncoding(unit->encoding());
        unitCopy->setFileName(unit->fileName());
        mUnits.append(unitCopy);
    }
}

void ProjectFilesWidget::disableFileOptions()
{
    ui->grpFileOptions->setEnabled(false);
    ui->spinPriority->setValue(0);
    ui->chkCompile->setChecked(false);
    ui->chkLink->setChecked(false);
    ui->chkCompileAsCPP->setChecked(false);
    ui->chkOverrideBuildCommand->setChecked(false);
    ui->txtBuildCommand->setPlainText("");
}

void ProjectFilesWidget::loadUnitEncoding(PProjectUnit unit)
{
    if (unit->encoding() == ENCODING_PROJECT
            || unit->encoding() == ENCODING_SYSTEM_DEFAULT
            || unit->encoding() == ENCODING_UTF8) {
        int index =ui->cbEncoding->findData(unit->encoding());
        ui->cbEncoding->setCurrentIndex(index);
        ui->cbEncodingDetail->clear();
        ui->cbEncodingDetail->setVisible(false);
    } else {
        QString encoding = unit->encoding();
        QString language = pCharsetInfoManager->findLanguageByCharsetName(encoding);
        ui->cbEncoding->setCurrentText(language);
        ui->cbEncodingDetail->setVisible(true);
        ui->cbEncodingDetail->clear();
        QList<PCharsetInfo> infos = pCharsetInfoManager->findCharsetsByLanguageName(language);
        foreach (const PCharsetInfo& info, infos) {
            ui->cbEncodingDetail->addItem(info->name);
        }
        ui->cbEncodingDetail->setCurrentText(encoding);
    }
}

void ProjectFilesWidget::on_treeProject_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) {
        disableFileOptions();
        return ;
    }
    ProjectModelNode* node = static_cast<ProjectModelNode*>(index.internalPointer());
    if (!node) {
        disableFileOptions();
        return;
    }
    PProjectUnit unit = currentUnit();
    if (unit) {
        disconnectInputs();
        ui->grpFileOptions->setEnabled(true);
        ui->spinPriority->setValue(unit->priority());
        ui->chkCompile->setChecked(unit->compile());
        ui->chkLink->setChecked(unit->link());
        ui->chkCompileAsCPP->setChecked(unit->compileCpp());
        ui->chkOverrideBuildCommand->setChecked(unit->overrideBuildCmd());
        ui->txtBuildCommand->setPlainText(unit->buildCmd());
        ui->txtBuildCommand->setEnabled(ui->chkOverrideBuildCommand->isChecked());
        loadUnitEncoding(unit);
        connectInputs();
        disconnectAbstractItemView(ui->treeProject);
    } else {
        disableFileOptions();
    }
}


void ProjectFilesWidget::on_spinPriority_valueChanged(int)
{
    PProjectUnit unit = currentUnit();
    if(!unit)
        return;
    unit->setPriority(ui->spinPriority->value());
}


void ProjectFilesWidget::on_chkCompile_stateChanged(int)
{
    PProjectUnit unit = currentUnit();
    if(!unit)
        return;
    unit->setCompile(ui->chkCompile->isChecked());
}


void ProjectFilesWidget::on_chkLink_stateChanged(int)
{
    PProjectUnit unit = currentUnit();
    if(!unit)
        return;
    unit->setLink(ui->chkLink->isChecked());
}


void ProjectFilesWidget::on_chkCompileAsCPP_stateChanged(int )
{
    PProjectUnit unit = currentUnit();
    if(!unit)
        return;
    unit->setCompileCpp(ui->chkCompileAsCPP->isChecked());
}


void ProjectFilesWidget::on_chkOverrideBuildCommand_stateChanged(int )
{
    PProjectUnit unit = currentUnit();
    if(!unit)
        return;
    unit->setOverrideBuildCmd(ui->chkOverrideBuildCommand->isChecked());
    ui->txtBuildCommand->setEnabled(ui->chkOverrideBuildCommand->isChecked());
}


void ProjectFilesWidget::on_txtBuildCommand_textChanged()
{
    PProjectUnit unit = currentUnit();
    if(!unit)
        return;
    unit->setBuildCmd(ui->txtBuildCommand->toPlainText());
}


void ProjectFilesWidget::on_cbEncoding_currentTextChanged(const QString &)
{
    QString userData = ui->cbEncoding->currentData().toString();
    if (userData == ENCODING_PROJECT
            || userData == ENCODING_SYSTEM_DEFAULT
            || userData == ENCODING_UTF8) {
        PProjectUnit unit = currentUnit();
        if(!unit)
            return;
        unit->setEncoding(userData.toUtf8());
        ui->cbEncodingDetail->setVisible(false);
        ui->cbEncodingDetail->clear();
    } else {
        ui->cbEncodingDetail->setVisible(true);
        ui->cbEncodingDetail->clear();
        QList<PCharsetInfo> infos = pCharsetInfoManager->findCharsetsByLanguageName(userData);
        foreach (const PCharsetInfo& info, infos) {
            ui->cbEncodingDetail->addItem(info->name);
        }
    }
}


void ProjectFilesWidget::on_treeProject_clicked(const QModelIndex &index)
{
    on_treeProject_doubleClicked(index);
}

void ProjectFilesWidget::init()
{
    std::shared_ptr<Project> project = pMainWindow->project();
    ui->spinPriority->setMinimum(0);
    ui->spinPriority->setMaximum(9999);
    ui->cbEncodingDetail->setVisible(false);
    ui->cbEncoding->clear();
    if (project->options().encoding==ENCODING_SYSTEM_DEFAULT) {
        ui->cbEncoding->addItem(tr("Project(%1)").arg(tr("ANSI"),ENCODING_PROJECT));
    } else {
        ui->cbEncoding->addItem(tr("Project(%1)").arg(QString(project->options().encoding)),ENCODING_PROJECT);
    }
    ui->cbEncoding->addItem(tr("System Default(%1)").arg(QString(pCharsetInfoManager->getDefaultSystemEncoding())),ENCODING_SYSTEM_DEFAULT);
    ui->cbEncoding->addItem(tr("UTF-8"),ENCODING_UTF8);
    foreach (const QString& langName, pCharsetInfoManager->languageNames()) {
        ui->cbEncoding->addItem(langName,langName);
    }
    copyUnits();
    QItemSelectionModel *m=ui->treeProject->selectionModel();
    ui->treeProject->setModel(project->model());
    delete m;
    SettingsWidget::init();
}

void ProjectFilesWidget::on_cbEncodingDetail_currentTextChanged(const QString &)
{
    PProjectUnit unit = currentUnit();
    if(!unit)
        return;
    if (ui->cbEncodingDetail->currentText().isEmpty())
        return;
    unit->setEncoding(ui->cbEncodingDetail->currentText().toUtf8());
}

