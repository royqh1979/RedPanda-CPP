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

#include "settings.h"
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

private:
    auto resolveExecArguments(const QString &terminalPath, const QString &argsPatter)
        -> std::tuple<QString, QStringList, PNonExclusiveTemporaryFileOwner>;
    void updateCommandPreview(const QString &terminalPath, const QString &argsPatter);
    void autoDetectAndUpdateArgumentsPattern(const QString &terminalPath);

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
    void on_txtArgsPattern_textChanged(const QString &argsPattern);
    void on_btnAutoDetectArgsPattern_clicked();
    void on_btnTest_clicked();
};

#endif // ENVIRONMENTPROGRAMSWIDGET_H
