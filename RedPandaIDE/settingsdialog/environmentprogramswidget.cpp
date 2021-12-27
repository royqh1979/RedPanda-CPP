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

#include <QFileDialog>

EnvironmentProgramsWidget::EnvironmentProgramsWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentProgramsWidget)
{
    ui->setupUi(this);
}

EnvironmentProgramsWidget::~EnvironmentProgramsWidget()
{
    delete ui;
}

void EnvironmentProgramsWidget::doLoad()
{
    ui->txtTerminal->setText(pSettings->environment().terminalPath());
}

void EnvironmentProgramsWidget::doSave()
{
    pSettings->environment().setTerminalPath(ui->txtTerminal->text());
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
