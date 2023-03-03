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
#include "../problems/ojproblemset.h"

OJProblemPropertyWidget::OJProblemPropertyWidget(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OJProblemPropertyWidget)
{
    ui->setupUi(this);
    QFont f = ui->lbName->font();
    f.setPixelSize(f.pixelSize()+2);
    f.setBold(true);
    ui->lbName->setFont(f);
    ui->cbTimeLimitUnit->addItem(tr("sec"));
    ui->cbTimeLimitUnit->addItem(tr("ms"));
    ui->cbMemoryLimitUnit->addItem(tr("KB"));
    ui->cbMemoryLimitUnit->addItem(tr("MB"));
    ui->cbMemoryLimitUnit->addItem(tr("GB"));
    ui->txtDescription->setFocus();
}

OJProblemPropertyWidget::~OJProblemPropertyWidget()
{
    delete ui;
}

void OJProblemPropertyWidget::loadFromProblem(POJProblem problem)
{
    if (!problem)
        return;
    ui->lbName->setText(problem->name);
    ui->txtURL->setText(problem->url);
    ui->txtDescription->setHtml(problem->description);
    ui->spinMemoryLimit->setValue(problem->memoryLimit);
    ui->spinTimeLimit->setValue(problem->timeLimit);
    switch(problem->timeLimitUnit) {
    case ProblemTimeLimitUnit::Seconds:
        ui->cbTimeLimitUnit->setCurrentText(tr("sec"));
        break;
    case ProblemTimeLimitUnit::Milliseconds:
        ui->cbTimeLimitUnit->setCurrentText(tr("ms"));
        break;
    }
    switch(problem->memoryLimitUnit) {
    case ProblemMemoryLimitUnit::KB:
        ui->cbMemoryLimitUnit->setCurrentText(tr("KB"));
        break;
    case ProblemMemoryLimitUnit::MB:
        ui->cbMemoryLimitUnit->setCurrentText(tr("MB"));
        break;
    case ProblemMemoryLimitUnit::GB:
        ui->cbMemoryLimitUnit->setCurrentText(tr("GB"));
        break;
    }
}

void OJProblemPropertyWidget::saveToProblem(POJProblem problem)
{
    if (!problem)
        return;
    problem->name = ui->lbName->text();
    problem->url = ui->txtURL->text();
    problem->description = ui->txtDescription->toHtml();
    problem->memoryLimit = ui->spinMemoryLimit->value();
    problem->timeLimit = ui->spinTimeLimit->value();
    if (ui->cbTimeLimitUnit->currentText()==tr("sec"))
        problem->timeLimitUnit = ProblemTimeLimitUnit::Seconds;
    else
        problem->timeLimitUnit = ProblemTimeLimitUnit::Milliseconds;
    if (ui->cbMemoryLimitUnit->currentText()==tr("KB"))
        problem->memoryLimitUnit = ProblemMemoryLimitUnit::KB;
    else if (ui->cbMemoryLimitUnit->currentText()==tr("MB"))
        problem->memoryLimitUnit = ProblemMemoryLimitUnit::MB;
    else
        problem->memoryLimitUnit = ProblemMemoryLimitUnit::GB;
}

void OJProblemPropertyWidget::on_btnOk_clicked()
{
    this->accept();
}


void OJProblemPropertyWidget::on_btnCancel_clicked()
{
    this->reject();
}

