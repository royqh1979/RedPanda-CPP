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
#include "environmentappearancewidget.h"
#include "ui_environmentappearancewidget.h"

#include <QApplication>
#include <QMessageBox>
#include <QStyleFactory>
#include "../settings.h"
#include "../mainwindow.h"
#include "../thememanager.h"
#include "../iconsmanager.h"

EnvironmentAppearanceWidget::EnvironmentAppearanceWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentAppearanceWidget)
{
    ui->setupUi(this);
}

EnvironmentAppearanceWidget::~EnvironmentAppearanceWidget()
{
    delete ui;
}

void EnvironmentAppearanceWidget::doLoad()
{
    for (int i=0; i<ui->cbTheme->count();i++) {
        if (ui->cbTheme->itemData(i) == pSettings->environment().theme()) {
            ui->cbTheme->setCurrentIndex(i);
            break;
        }
    }
    ui->cbFont->setCurrentFont(QFont(pSettings->environment().interfaceFont()));
    ui->spinFontSize->setValue(pSettings->environment().interfaceFontSize());
    for (int i=0; i<ui->cbIconSet->count();i++) {
        if (ui->cbIconSet->itemData(i) == pSettings->environment().iconSet()) {
            ui->cbIconSet->setCurrentIndex(i);
            break;
        }
    }
    ui->spinZoomFactor->setValue(pSettings->environment().iconZoomFactor());
    ui->chkUseCustomIconSet->setChecked(pSettings->environment().useCustomIconSet());

    for (int i=0;i<ui->cbLanguage->count();i++) {
        if (ui->cbLanguage->itemData(i) == pSettings->environment().language()) {
            ui->cbLanguage->setCurrentIndex(i);
            break;
        }
    }
}

void EnvironmentAppearanceWidget::doSave()
{
    if (pSettings->environment().theme()!=ui->cbTheme->currentData().toString()) {
        ThemeManager themeManager;
        PAppTheme appTheme = themeManager.theme(ui->cbTheme->currentData().toString());
        if (appTheme && !appTheme->defaultColorScheme().isEmpty()) {
            pSettings->editor().setColorScheme(appTheme->defaultColorScheme());
            pSettings->editor().save();
            pMainWindow->updateEditorColorSchemes();
        }
    }
    pSettings->environment().setTheme(ui->cbTheme->currentData().toString());
    pSettings->environment().setInterfaceFont(ui->cbFont->currentFont().family());
    pSettings->environment().setInterfaceFontSize(ui->spinFontSize->value());
    pSettings->environment().setLanguage(ui->cbLanguage->currentData().toString());
    pSettings->environment().setIconSet(ui->cbIconSet->currentData().toString());
    pSettings->environment().setIconZoomFactor(ui->spinZoomFactor->value());

    pSettings->environment().setUseCustomIconSet(ui->chkUseCustomIconSet->isChecked());

    pSettings->environment().save();
    pMainWindow->applySettings();
}

void EnvironmentAppearanceWidget::updateIcons(const QSize & size)
{
    Q_UNUSED(size);
    pIconsManager->setIcon(ui->btnCustomize, IconsManager::ACTION_EDIT_COPY);
    pIconsManager->setIcon(ui->btnOpenCustomThemeFolder, IconsManager::ACTION_MISC_FOLDER);
    pIconsManager->setIcon(ui->btnRemoveCustomTheme, IconsManager::ACTION_MISC_REMOVE);
}

void EnvironmentAppearanceWidget::init()
{
    ThemeManager themeManager;
    QList<PAppTheme> appThemes = themeManager.getThemes();
    foreach(const PAppTheme& appTheme, appThemes) {
        ui->cbTheme->addItem(appTheme->categoryIcon() + " " + appTheme->displayName(), appTheme->name());
    }
    ui->cbLanguage->addItem(tr("English"),"en");
    ui->cbLanguage->addItem(tr("Portuguese"),"pt_BR");
    ui->cbLanguage->addItem(tr("Simplified Chinese"),"zh_CN");
    ui->cbLanguage->addItem(tr("Traditional Chinese"),"zh_TW");
    QList<PIconSet> iconSets = pIconsManager->listIconSets();
    foreach(const PIconSet& iconSet, iconSets) {
        ui->cbIconSet->addItem(iconSet->displayName,iconSet->name);
    }
    SettingsWidget::init();
}

void EnvironmentAppearanceWidget::on_cbTheme_currentIndexChanged(int /* index */)
{
    ThemeManager themeManager;
    PAppTheme appTheme = themeManager.theme(ui->cbTheme->currentData().toString());
    ui->btnCustomize->setVisible(appTheme->category() == AppTheme::ThemeCategory::BuiltIn);
    ui->btnOpenCustomThemeFolder->setVisible(appTheme->category() == AppTheme::ThemeCategory::Custom);
    ui->btnRemoveCustomTheme->setVisible(appTheme->category() == AppTheme::ThemeCategory::Custom);
    if(!appTheme->defaultIconSet().isEmpty()) {
        for (int i=0; i<ui->cbIconSet->count();i++) {
            if (ui->cbIconSet->itemData(i) == appTheme->defaultIconSet()) {
                ui->cbIconSet->setCurrentIndex(i);
                break;
            }
        }
    }
}


void EnvironmentAppearanceWidget::on_btnCustomize_clicked()
{
    ThemeManager themeManager;
    PAppTheme appTheme = themeManager.theme(ui->cbTheme->currentData().toString());
    QString customThemeFolder = pSettings->dirs().config(Settings::Dirs::DataType::Theme);
    QDir dir{customThemeFolder};
    if (!dir.exists()) {
        dir.mkpath(customThemeFolder);
    }
    appTheme->copyTo(customThemeFolder);
    refreshThemeList(appTheme->name());
}


void EnvironmentAppearanceWidget::on_btnOpenCustomThemeFolder_clicked()
{
    QString customThemeFolder = pSettings->dirs().config(Settings::Dirs::DataType::Theme);
    openFileFolderInExplorer(customThemeFolder);
}

void EnvironmentAppearanceWidget::refreshThemeList(const QString &currentThemeName)
{
    ThemeManager themeManager;
    ui->cbTheme->clear();
    QList<PAppTheme> appThemes = themeManager.getThemes();
    for (int i=0; i<appThemes.count();i++) {
        const PAppTheme& appTheme =appThemes[i];
        ui->cbTheme->addItem(appTheme->categoryIcon() + " " + appTheme->displayName(), appTheme->name());
        if (appTheme->name() == currentThemeName)
            ui->cbTheme->setCurrentIndex(i);
    }
}


void EnvironmentAppearanceWidget::on_btnRemoveCustomTheme_clicked()
{
    ThemeManager themeManager;
    PAppTheme appTheme = themeManager.theme(ui->cbTheme->currentData().toString());
    if (appTheme->category() != AppTheme::ThemeCategory::Custom)
        return;
    QFile::remove(appTheme->filename());
    refreshThemeList(appTheme->name());
}

