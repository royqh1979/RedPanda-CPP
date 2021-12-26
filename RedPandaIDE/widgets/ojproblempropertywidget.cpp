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
#include "ojproblempropertywidget.h"
#include "ui_ojproblempropertywidget.h"

OJProblemPropertyWidget::OJProblemPropertyWidget(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OJProblemPropertyWidget)
{
    ui->setupUi(this);
}

OJProblemPropertyWidget::~OJProblemPropertyWidget()
{
    delete ui;
}

void OJProblemPropertyWidget::setName(const QString &name)
{
    QFont f = ui->lbName->font();
    f.setPixelSize(f.pixelSize()+2);
    f.setBold(true);
    ui->lbName->setFont(f);
    ui->lbName->setText(name);
}

void OJProblemPropertyWidget::setUrl(const QString &url)
{
    ui->txtURL->setText(url);
}

void OJProblemPropertyWidget::setDescription(const QString &description)
{
    ui->txtDescription->setHtml(description);
}

QString OJProblemPropertyWidget::name()
{
    return ui->lbName->text();
}

QString OJProblemPropertyWidget::url()
{
    return ui->txtURL->text();
}

QString OJProblemPropertyWidget::description()
{
    return ui->txtDescription->toHtml();
}

void OJProblemPropertyWidget::on_btnOk_clicked()
{
    this->accept();
}


void OJProblemPropertyWidget::on_btnCancel_clicked()
{
    this->reject();
}

