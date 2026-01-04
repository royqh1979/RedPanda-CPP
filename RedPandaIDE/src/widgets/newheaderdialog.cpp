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
#include "newheaderdialog.h"
#include "ui_newheaderdialog.h"
#include "../iconsmanager.h"
#include "../settings.h"

#include <QFileDialog>

NewHeaderDialog::NewHeaderDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewHeaderDialog)
{
    setWindowFlag(Qt::WindowContextHelpButtonHint,false);
    ui->setupUi(this);
    resize(pSettings->ui().newHeaderDialogWidth(),pSettings->ui().newHeaderDialogHeight());
    onUpdateIcons();
    connect(pIconsManager,&IconsManager::actionIconsUpdated,
            this, &NewHeaderDialog::onUpdateIcons);
    ui->txtHeader->setFocus();
}

NewHeaderDialog::~NewHeaderDialog()
{
    delete ui;
}

QString NewHeaderDialog::headerName() const
{
    return ui->txtHeader->text();
}

QString NewHeaderDialog::path() const
{
    return ui->txtPath->text();
}

void NewHeaderDialog::setHeaderName(const QString &name)
{
    ui->txtHeader->setText(name);
    int pos = name.lastIndexOf('.');
    if (pos>=0)
        ui->txtHeader->setSelection(0,pos);
    else
        ui->txtHeader->selectAll();
}

void NewHeaderDialog::setPath(const QString &location)
{
    ui->txtPath->setText(location);
}

void NewHeaderDialog::onUpdateIcons()
{
    pIconsManager->setIcon(ui->btnBrowse, IconsManager::ACTION_FILE_OPEN_FOLDER);
}

void NewHeaderDialog::closeEvent(QCloseEvent */*event*/)
{
    reject();
}

void NewHeaderDialog::on_btnCreate_clicked()
{
    accept();
}


void NewHeaderDialog::on_btnCancel_clicked()
{
    reject();
}


void NewHeaderDialog::on_btnBrowse_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(
                this,
                tr("Path"),
                ui->txtPath->text());
    ui->txtPath->setText(fileName);
}

