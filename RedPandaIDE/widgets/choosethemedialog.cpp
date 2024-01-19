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
#include "choosethemedialog.h"
#include "ui_choosethemedialog.h"

ChooseThemeDialog::ChooseThemeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ChooseThemeDialog)
{
    ui->setupUi(this);
#ifdef ENABLE_LUA_ADDON
    ui->rbAuto->setVisible(true);
#else
    ui->rbAuto->setVisible(false);
#endif
    ui->rbDark->setChecked(true);
    ui->rbCpp->setChecked(true);
}

ChooseThemeDialog::~ChooseThemeDialog()
{
    delete ui;
}

ChooseThemeDialog::Theme ChooseThemeDialog::theme()
{
#ifdef ENABLE_LUA_ADDON
    if (ui->rbAuto->isChecked())
        return Theme::AutoFollowSystem;
#endif
    if (ui->rbDark->isChecked())
        return Theme::Dark;
    if (ui->rbLight->isChecked())
        return Theme::Light;
    return Theme::Unknown;
}

ChooseThemeDialog::Language ChooseThemeDialog::language()
{
    return ui->rbCpp->isChecked()?Language::CPlusPlus:Language::C;
}

void ChooseThemeDialog::on_btnOk_clicked()
{
    accept();
}
