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
#include "executorgeneralwidget.h"
#include "ui_executorgeneralwidget.h"
#include "../settings.h"
#include "../iconsmanager.h"
#include "../systemconsts.h"
#include "utils.h"
#include "utils/font.h"
#include "utils/parsearg.h"

#include <QFileDialog>
#include <QJsonDocument>
#include <QJsonArray>

ExecutorGeneralWidget::ExecutorGeneralWidget(const QString& name, const QString& group, QWidget *parent):
    SettingsWidget(name,group,parent),
    ui(new Ui::ExecutorGeneralWidget)
{
    ui->setupUi(this);
    ui->txtParsedArgsInJson->setFont(defaultMonoFont());
#ifdef Q_OS_WIN
    ui->chkVTSeq->setVisible(true);
#else
    ui->chkVTSeq->setVisible(false);
#endif
}

ExecutorGeneralWidget::~ExecutorGeneralWidget()
{
    delete ui;
}

void ExecutorGeneralWidget::doLoad()
{
    ui->chkPauseConsole->setChecked(pSettings->executor().pauseConsole());
#ifdef Q_OS_WIN
    ui->chkVTSeq->setChecked(pSettings->executor().enableVirualTerminalSequence());
#endif
    ui->chkMinimizeOnRun->setChecked(pSettings->executor().minimizeOnRun());
    ui->grpExecuteParameters->setChecked(pSettings->executor().useParams());
    ui->txtExecuteParamaters->setText(pSettings->executor().params());
    ui->grpRedirectInput->setChecked(pSettings->executor().redirectInput());
    ui->txtRedirectInputFile->setText(pSettings->executor().inputFilename());
}

void ExecutorGeneralWidget::doSave()
{
    pSettings->executor().setPauseConsole(ui->chkPauseConsole->isChecked());
#ifdef Q_OS_WIN
    pSettings->executor().setEnableVirualTerminalSequence(ui->chkVTSeq->isChecked());
#endif
    pSettings->executor().setMinimizeOnRun(ui->chkMinimizeOnRun->isChecked());
    pSettings->executor().setUseParams(ui->grpExecuteParameters->isChecked());
    pSettings->executor().setParams(ui->txtExecuteParamaters->text());
    pSettings->executor().setRedirectInput(ui->grpRedirectInput->isChecked());
    pSettings->executor().setInputFilename(ui->txtRedirectInputFile->text());

    pSettings->executor().save();
}

void ExecutorGeneralWidget::on_btnBrowse_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Choose input file"),
                QString(),
                tr("All files (%1)").arg(ALL_FILE_WILDCARD));
    if (!filename.isEmpty() && fileExists(filename)) {
        ui->txtRedirectInputFile->setText(filename);
    }
}

void ExecutorGeneralWidget::updateIcons(const QSize &/*size*/)
{
    pIconsManager->setIcon(ui->btnBrowse,IconsManager::ACTION_FILE_OPEN_FOLDER);
}


void ExecutorGeneralWidget::on_txtExecuteParamaters_textChanged(const QString &commandLine)
{
    QStringList parsed = parseArgumentsWithoutVariables(commandLine);
    QJsonArray obj = QJsonArray::fromStringList(parsed);
    ui->txtParsedArgsInJson->setText(QJsonDocument{obj}.toJson());
}
