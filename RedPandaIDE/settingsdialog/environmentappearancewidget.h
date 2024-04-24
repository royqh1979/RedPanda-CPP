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
#ifndef ENVIRONMENTAPPEARANCEWIDGET_H
#define ENVIRONMENTAPPEARANCEWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EnvironmentAppearanceWidget;
}

class EnvironmentAppearanceWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EnvironmentAppearanceWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EnvironmentAppearanceWidget();

private:
    Ui::EnvironmentAppearanceWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
    void updateIcons(const QSize &size) override;

    // SettingsWidget interface
public:
    void init() override;
private slots:
    void on_cbTheme_currentIndexChanged(int index);
    void on_btnCustomize_clicked();
    void on_btnOpenCustomThemeFolder_clicked();
    void on_btnRemoveCustomTheme_clicked();

private:
    void refreshThemeList(const QString& currentThemeName);

};

#endif // ENVIRONMENTAPPEARANCEWIDGET_H
