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
#ifndef ENVIRONMENTPROGRAMSWIDGET_H
#define ENVIRONMENTPROGRAMSWIDGET_H

#include "settingswidget.h"
#include "utils.h"

namespace Ui {
class EnvironmentProgramsWidget;
}

class EnvironmentProgramsWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EnvironmentProgramsWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EnvironmentProgramsWidget();
    void hideMacosSpecificPattern();

private:
    void testTerminal(const TerminalEmulatorArgumentsPattern &pattern);

private:
    Ui::EnvironmentProgramsWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
    void updateIcons(const QSize &size) override;
private slots:
    void on_btnChooseTerminal_clicked();
    void on_txtTerminal_textChanged(const QString &terminalPath);
    void on_pbImplicitSystem_clicked();
    void on_pbMinusEAppendArgs_clicked();
    void on_pbMinusXAppendArgs_clicked();
    void on_pbMinusMinusAppendArgs_clicked();
    void on_pbMinusEAppendCommandLine_clicked();
    void on_pbWriteCommandLineToTempFileThenTempFilename_clicked();
};

#endif // ENVIRONMENTPROGRAMSWIDGET_H
