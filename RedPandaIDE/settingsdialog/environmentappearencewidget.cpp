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
#include "environmentappearencewidget.h"
#include "ui_environmentappearencewidget.h"

#include <QApplication>
#include <QStyleFactory>
#include "../settings.h"
#include "../mainwindow.h"
#include "../thememanager.h"

EnvironmentAppearenceWidget::EnvironmentAppearenceWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentAppearenceWidget)
{
    ui->setupUi(this);
    ui->cbTheme->addItem("default");
    ui->cbTheme->addItem("dark");
//    ui->cbTheme->addItem("dracula");
//    ui->cbTheme->addItem("light");
//    QStyleFactory factory;
//    for (QString name:factory.keys()) {
//        ui->cbTheme->addItem(name);
//    }
    ui->cbLanguage->addItem(tr("English"),"en");
    ui->cbLanguage->addItem(tr("Simplified Chinese"),"zh_CN");
    ui->cbIconSet->addItem("newlook");
}

EnvironmentAppearenceWidget::~EnvironmentAppearenceWidget()
{
    delete ui;
}

void EnvironmentAppearenceWidget::doLoad()
{
    ui->cbTheme->setCurrentText(pSettings->environment().theme());
    ui->cbFont->setCurrentFont(QFont(pSettings->environment().interfaceFont()));
    ui->spinFontSize->setValue(pSettings->environment().interfaceFontSize());
    ui->cbIconSet->setCurrentText(pSettings->environment().iconSet());

    for (int i=0;i<ui->cbLanguage->count();i++) {
        if (ui->cbLanguage->itemData(i) == pSettings->environment().language()) {
            ui->cbLanguage->setCurrentIndex(i);
            break;
        }
    }
}

void EnvironmentAppearenceWidget::doSave()
{
    if (pSettings->environment().theme()!=ui->cbTheme->currentText()) {
        ThemeManager themeManager;
        PAppTheme appTheme = themeManager.theme(ui->cbTheme->currentText());
        if (appTheme && !appTheme->defaultColorScheme().isEmpty()) {
            pSettings->editor().setColorScheme(appTheme->defaultColorScheme());
            pMainWindow->updateEditorColorSchemes();
        }
    }
    pSettings->environment().setTheme(ui->cbTheme->currentText());
    pSettings->environment().setInterfaceFont(ui->cbFont->currentFont().family());
    pSettings->environment().setInterfaceFontSize(ui->spinFontSize->value());
    pSettings->environment().setLanguage(ui->cbLanguage->currentData().toString());
    pSettings->environment().setIconSet(ui->cbIconSet->currentText());
    pSettings->editor().save();
    pSettings->environment().save();
    pMainWindow->applySettings();
}
