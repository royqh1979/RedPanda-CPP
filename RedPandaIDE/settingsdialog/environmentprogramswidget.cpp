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

#include <QFileDialog>

EnvironmentProgramsWidget::EnvironmentProgramsWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentProgramsWidget)
{
    ui->setupUi(this);
    QFont monoFont(DEFAULT_MONO_FONT);
    ui->rbImplicitSystem->setFont(monoFont);
    ui->rbMinusEAppendArgs->setFont(monoFont);
    ui->rbMinusXAppendArgs->setFont(monoFont);
    ui->rbMinusMinusAppendArgs->setFont(monoFont);
    ui->rbMinusEAppendCommandLine->setFont(monoFont);
    ui->rbWriteCommandLineToTempFileThenTempFilename->setFont(monoFont);
#ifndef Q_OS_MACOS
    hideMacosSpecificPattern();
#endif
}

EnvironmentProgramsWidget::~EnvironmentProgramsWidget()
{
    delete ui;
}

void EnvironmentProgramsWidget::hideMacosSpecificPattern()
{
    ui->rbWriteCommandLineToTempFileThenTempFilename->setVisible(false);
    ui->rbWriteCommandLineToTempFileThenTempFilename->setEnabled(false);
    ui->pbWriteCommandLineToTempFileThenTempFilename->setVisible(false);
    ui->pbWriteCommandLineToTempFileThenTempFilename->setEnabled(false);
}

void EnvironmentProgramsWidget::testTerminal(const TerminalEmulatorArgumentsPattern &pattern)
{
    auto [filename, arguments, fileOwner] = wrapCommandForTerminalEmulator(ui->txtTerminal->text(), pattern, {defaultShell(), "-c", "echo hello; sleep 3"});
    ExecutableRunner runner(filename, arguments, "", nullptr);
    runner.start();
    runner.wait();
}

void EnvironmentProgramsWidget::doLoad()
{
    ui->txtTerminal->setText(pSettings->environment().terminalPath());
    switch (pSettings->environment().terminalArgumentsPattern()) {
    case TerminalEmulatorArgumentsPattern::ImplicitSystem:
        ui->rbImplicitSystem->setChecked(true);
        break;
    case TerminalEmulatorArgumentsPattern::MinusEAppendArgs:
        ui->rbMinusEAppendArgs->setChecked(true);
        break;
    case TerminalEmulatorArgumentsPattern::MinusXAppendArgs:
        ui->rbMinusXAppendArgs->setChecked(true);
        break;
    case TerminalEmulatorArgumentsPattern::MinusMinusAppendArgs:
        ui->rbMinusMinusAppendArgs->setChecked(true);
        break;
    case TerminalEmulatorArgumentsPattern::MinusEAppendCommandLine:
        ui->rbMinusEAppendCommandLine->setChecked(true);
        break;
    case TerminalEmulatorArgumentsPattern::WriteCommandLineToTempFileThenTempFilename:
        ui->rbWriteCommandLineToTempFileThenTempFilename->setChecked(true);
        break;
    }
}

void EnvironmentProgramsWidget::doSave()
{
    pSettings->environment().setTerminalPath(ui->txtTerminal->text());
    if (ui->rbImplicitSystem->isChecked())
        pSettings->environment().setTerminalArgumentsPattern(TerminalEmulatorArgumentsPattern::ImplicitSystem);
    if (ui->rbMinusEAppendArgs->isChecked())
        pSettings->environment().setTerminalArgumentsPattern(TerminalEmulatorArgumentsPattern::MinusEAppendArgs);
    if (ui->rbMinusXAppendArgs->isChecked())
        pSettings->environment().setTerminalArgumentsPattern(TerminalEmulatorArgumentsPattern::MinusXAppendArgs);
    if (ui->rbMinusMinusAppendArgs->isChecked())
        pSettings->environment().setTerminalArgumentsPattern(TerminalEmulatorArgumentsPattern::MinusMinusAppendArgs);
    if (ui->rbMinusEAppendCommandLine->isChecked())
        pSettings->environment().setTerminalArgumentsPattern(TerminalEmulatorArgumentsPattern::MinusEAppendCommandLine);
    if (ui->rbWriteCommandLineToTempFileThenTempFilename->isChecked())
        pSettings->environment().setTerminalArgumentsPattern(TerminalEmulatorArgumentsPattern::WriteCommandLineToTempFileThenTempFilename);
    pSettings->environment().save();
}

void EnvironmentProgramsWidget::updateIcons(const QSize &)
{
    pIconsManager->setIcon(ui->btnChooseTerminal,IconsManager::ACTION_FILE_OPEN_FOLDER);
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
    }
}

void EnvironmentProgramsWidget::on_txtTerminal_textChanged(const QString &terminalPath)
{
    QString terminalPathForExec;
    if (getPathUnixExecSemantics(terminalPath) == UnixExecSemantics::RelativeToCwd) {
        QDir appDir(pSettings->dirs().appDir());
        terminalPathForExec = appDir.absoluteFilePath(terminalPath);
    } else
        terminalPathForExec = terminalPath;
    QString terminalPathEscaped = escapeArgument(terminalPathForExec, true);
    QString shell = defaultShell();
    QStringList execArgs{shell, "-c", "echo hello; sleep 3"};

    auto displayCommand = [this, &execArgs](const TerminalEmulatorArgumentsPattern &pattern) {
        auto [filename, arguments, fileOwner] = wrapCommandForTerminalEmulator(ui->txtTerminal->text(), pattern, execArgs);
        for (auto &arg : arguments)
            arg = escapeArgument(arg, false);
        return escapeArgument(filename, true) + " " + arguments.join(' ');
    };

    ui->rbImplicitSystem->setText(displayCommand(TerminalEmulatorArgumentsPattern::ImplicitSystem));
    ui->rbMinusEAppendArgs->setText(displayCommand(TerminalEmulatorArgumentsPattern::MinusEAppendArgs));
    ui->rbMinusXAppendArgs->setText(displayCommand(TerminalEmulatorArgumentsPattern::MinusXAppendArgs));
    ui->rbMinusMinusAppendArgs->setText(displayCommand(TerminalEmulatorArgumentsPattern::MinusMinusAppendArgs));
    ui->rbMinusEAppendCommandLine->setText(displayCommand(TerminalEmulatorArgumentsPattern::MinusEAppendCommandLine));
    if (ui->rbWriteCommandLineToTempFileThenTempFilename->isEnabled())
        ui->rbWriteCommandLineToTempFileThenTempFilename->setText(displayCommand(TerminalEmulatorArgumentsPattern::WriteCommandLineToTempFileThenTempFilename));
}

void EnvironmentProgramsWidget::on_pbImplicitSystem_clicked()
{
    testTerminal(TerminalEmulatorArgumentsPattern::ImplicitSystem);
}

void EnvironmentProgramsWidget::on_pbMinusEAppendArgs_clicked()
{
    testTerminal(TerminalEmulatorArgumentsPattern::MinusEAppendArgs);
}

void EnvironmentProgramsWidget::on_pbMinusXAppendArgs_clicked()
{
    testTerminal(TerminalEmulatorArgumentsPattern::MinusXAppendArgs);
}

void EnvironmentProgramsWidget::on_pbMinusMinusAppendArgs_clicked()
{
    testTerminal(TerminalEmulatorArgumentsPattern::MinusMinusAppendArgs);
}

void EnvironmentProgramsWidget::on_pbMinusEAppendCommandLine_clicked()
{
    testTerminal(TerminalEmulatorArgumentsPattern::MinusEAppendCommandLine);
}

void EnvironmentProgramsWidget::on_pbWriteCommandLineToTempFileThenTempFilename_clicked()
{
    testTerminal(TerminalEmulatorArgumentsPattern::WriteCommandLineToTempFileThenTempFilename);
}

