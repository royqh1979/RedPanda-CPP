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
#include "environmentdatasizewidget.h"
#include "ui_environmentdatasizewidget.h"
#include "../settings.h"
#include "../iconsmanager.h"
#include "../systemconsts.h"
#include "../compiler/executablerunner.h"
#include "utils.h"
#include "utils/escape.h"
#include "utils/font.h"

#include <QFileDialog>
#include <QMessageBox>

EnvironmentDataSizeWidget::EnvironmentDataSizeWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentDataSizeWidget)
{
    ui->setupUi(this);
    updateScalePreview(pSettings->environment().dataSizeDialect());
}

EnvironmentDataSizeWidget::~EnvironmentDataSizeWidget()
{
    delete ui;
}

void EnvironmentDataSizeWidget::updateScalePreview(DataSizeDialect dialect)
{
    QLocale locale = QLocale::system();
    QLocale::DataSizeFormats qtDialect = Settings::Environment::qtDataSizeDialect(dialect);
    QString previewText;
    auto append = [&] (QString display1, qint64 size1, QString display2, qint64 size2) {
        previewText += display1;
        previewText += " B = ";
        previewText += locale.formattedDataSize(size1, 2, qtDialect);
        previewText += ",    ";
        previewText += display2;
        previewText += " B = ";
        previewText += locale.formattedDataSize(size2, 2, qtDialect);
        previewText += "\n";
    };
    append("10³", 1'000, "2¹⁰", 1LL << 10);
    append("10⁶", 1'000'000, "2²⁰", 1LL << 20);
    append("10⁹", 1'000'000'000, "2³⁰", 1LL << 30);
    append("10¹²", 1'000'000'000'000, "2⁴⁰", 1LL << 40);
    append("10¹⁵", 1'000'000'000'000'000, "2⁵⁰", 1LL << 50);
    append("10¹⁸", 1'000'000'000'000'000'000, "2⁶⁰", 1LL << 60);
    ui->tbScalePreview->setText(previewText);
}

void EnvironmentDataSizeWidget::doLoad()
{
    switch (pSettings->environment().dataSizeDialect()) {
        case DataSizeDialect::SiDecimal:
            ui->rbSiDecimal->setChecked(true);
            break;
        case DataSizeDialect::IecBinary:
            ui->rbIecBinary->setChecked(true);
            break;
        case DataSizeDialect::JedecBinary:
            ui->rbJedecBinary->setChecked(true);
            break;
    }
}

void EnvironmentDataSizeWidget::doSave()
{
    if (ui->rbSiDecimal->isChecked())
        pSettings->environment().setDataSizeDialect(DataSizeDialect::SiDecimal);
    else if (ui->rbIecBinary->isChecked())
        pSettings->environment().setDataSizeDialect(DataSizeDialect::IecBinary);
    else if (ui->rbJedecBinary->isChecked())
        pSettings->environment().setDataSizeDialect(DataSizeDialect::JedecBinary);
    pSettings->environment().save();
}

void EnvironmentDataSizeWidget::updateIcons(const QSize &)
{
}

void EnvironmentDataSizeWidget::on_rbSiDecimal_clicked()
{
    updateScalePreview(DataSizeDialect::SiDecimal);
}

void EnvironmentDataSizeWidget::on_rbIecBinary_clicked()
{
    updateScalePreview(DataSizeDialect::IecBinary);
}

void EnvironmentDataSizeWidget::on_rbJedecBinary_clicked()
{
    updateScalePreview(DataSizeDialect::JedecBinary);
}
