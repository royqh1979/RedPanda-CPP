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
#include "environmentprogramswidget.h"
#include "ui_environmentprogramswidget.h"
#include "../settings.h"
#include "../iconsmanager.h"
#include "../systemconsts.h"
#include "../compiler/executablerunner.h"
#include "utils.h"
#include "utils/escape.h"
#include "utils/font.h"

#include <QFileDialog>
#include <QMessageBox>

EnvironmentProgramsWidget::EnvironmentProgramsWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentProgramsWidget)
{
    ui->setupUi(this);
    ui->labelCmdPreviewResult->setFont(defaultMonoFont());
#ifndef Q_OS_WINDOWS
    ui->grpUseCustomTerminal->setCheckable(false);
#endif
}

EnvironmentProgramsWidget::~EnvironmentProgramsWidget()
{
    delete ui;
}

auto EnvironmentProgramsWidget::resolveExecArguments(const QString &terminalPath, const QString &argsPattern)
    -> std::tuple<QString, QStringList, PNonExclusiveTemporaryFileOwner>
{
    return wrapCommandForTerminalEmulator(terminalPath, argsPattern, platformCommandForTerminalArgsPreview());
}

void EnvironmentProgramsWidget::updateCommandPreview(const QString &terminalPath, const QString &argsPattern)
{
    auto [filename, arguments, fileOwner] = resolveExecArguments(terminalPath, argsPattern);
    for (auto &arg : arguments)
        arg = escapeArgument(arg, false, platformShellEscapeArgumentRule());

    ui->labelCmdPreviewResult->setPlainText(escapeArgument(filename, true, platformShellEscapeArgumentRule()) + " " + arguments.join(' '));
}

void EnvironmentProgramsWidget::autoDetectAndUpdateArgumentsPattern(const QString &terminalPath)
{
    const QString &executable = QFileInfo(terminalPath).fileName();
    const QString &pattern = pSettings->environment().queryPredefinedTerminalArgumentsPattern(executable);
    if (!pattern.isEmpty())
        ui->txtArgsPattern->setText(pattern);
    else
        QMessageBox::warning(nullptr,
                             QObject::tr("Auto Detection Failed"),
                             QObject::tr("Failed to detect terminal arguments pattern for “%1”.").arg(executable),
                             QMessageBox::Ok);
}

void EnvironmentProgramsWidget::doLoad()
{
#ifdef Q_OS_WINDOWS
    ui->grpUseCustomTerminal->setChecked(pSettings->environment().useCustomTerminal());
#endif
    ui->txtTerminal->setText(pSettings->environment().terminalPath());
    ui->txtArgsPattern->setText(pSettings->environment().terminalArgumentsPattern());
}

void EnvironmentProgramsWidget::doSave()
{
#ifdef Q_OS_WINDOWS
    pSettings->environment().setUseCustomTerminal(ui->grpUseCustomTerminal->isChecked());
#endif
    pSettings->environment().setTerminalPath(ui->txtTerminal->text());
    pSettings->environment().setTerminalArgumentsPattern(ui->txtArgsPattern->text());
    pSettings->environment().save();
}

void EnvironmentProgramsWidget::updateIcons(const QSize &)
{
    pIconsManager->setIcon(ui->btnChooseTerminal,IconsManager::ACTION_FILE_OPEN_FOLDER);
    pIconsManager->setIcon(ui->btnAutoDetectArgsPattern,IconsManager::ACTION_EDIT_SEARCH);
    pIconsManager->setIcon(ui->btnTest,IconsManager::ACTION_RUN_RUN);
}

void EnvironmentProgramsWidget::on_btnChooseTerminal_clicked()
{
    QString filename = QFileDialog::getOpenFileName(
                this,
                tr("Choose Terminal Program"),
                QString(),
                tr("All files (%1)").arg(ALL_FILE_WILDCARD));
    if (!filename.isEmpty() && fileExists(filename) ) {
        ui->txtTerminal->setText(filename);
        autoDetectAndUpdateArgumentsPattern(filename);
    }
}

void EnvironmentProgramsWidget::on_txtTerminal_textChanged(const QString &terminalPath)
{
    const QString &argsPattern = ui->txtArgsPattern->text();
    updateCommandPreview(terminalPath, argsPattern);
}

void EnvironmentProgramsWidget::on_txtArgsPattern_textChanged(const QString &argsPattern)
{
    const QString &terminalPath = ui->txtTerminal->text();
    updateCommandPreview(terminalPath, argsPattern);
}

void EnvironmentProgramsWidget::on_btnAutoDetectArgsPattern_clicked()
{
    const QString &terminalPath = ui->txtTerminal->text();
    autoDetectAndUpdateArgumentsPattern(terminalPath);
}

void EnvironmentProgramsWidget::on_btnTest_clicked()
{
    const QString &terminalPath = ui->txtTerminal->text();
    const QString &argsPattern = ui->txtArgsPattern->text();
    auto [filename, arguments, fileOwner] = resolveExecArguments(terminalPath, argsPattern);
    ExecutableRunner runner(filename, arguments, "", nullptr);
    runner.start();
    runner.wait();
}
