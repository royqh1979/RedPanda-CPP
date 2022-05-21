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
#include "newclassdialog.h"
#include "ui_newclassdialog.h"
#include "../iconsmanager.h"
#include "../settings.h"
#include <QFileDialog>

NewClassDialog::NewClassDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::NewClassDialog)
{
    setWindowFlag(Qt::WindowContextHelpButtonHint,false);
    ui->setupUi(this);
    resize(pSettings->ui().newClassDialogWidth(),pSettings->ui().newClassDialogHeight());
    onUpdateIcons();
    connect(pIconsManager,&IconsManager::actionIconsUpdated,
            this, &NewClassDialog::onUpdateIcons);
    ui->txtClassName->setFocus();
}

NewClassDialog::~NewClassDialog()
{
}

QString NewClassDialog::className() const
{
    return ui->txtClassName->text();
}

QString NewClassDialog::baseClass() const
{
    return ui->cbBaseClass->currentText();
}

QString NewClassDialog::headerName() const
{
    return ui->txtHeaderName->text();
}

QString NewClassDialog::sourceName() const
{
    return ui->txtSourceName->text();
}

QString NewClassDialog::path() const
{
    return ui->txtPath->text();
}

void NewClassDialog::setPath(const QString &location)
{
    ui->txtPath->setText(location);
}

void NewClassDialog::on_btnCancel_clicked()
{
    this->reject();
}

void NewClassDialog::closeEvent(QCloseEvent *event)
{
    this->reject();
}


void NewClassDialog::on_btnCreate_clicked()
{
    this->accept();
}

void NewClassDialog::onUpdateIcons()
{
    pIconsManager->setIcon(ui->btnBrowsePath, IconsManager::ACTION_FILE_OPEN_FOLDER);
}


void NewClassDialog::on_btnBrowsePath_clicked()
{
    QString fileName = QFileDialog::getExistingDirectory(
                this,
                tr("Path"),
                ui->txtPath->text());
    ui->txtPath->setText(fileName);
}


void NewClassDialog::on_txtClassName_textChanged(const QString &/* arg1 */)
{
    ui->txtHeaderName->setText(ui->txtClassName->text().toLower()+".h");
    ui->txtSourceName->setText(ui->txtClassName->text().toLower()+".cpp");
}

