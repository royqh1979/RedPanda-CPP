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
#ifndef ENVIRONMENTFOLDERSWIDGET_H
#define ENVIRONMENTFOLDERSWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EnvironmentFoldersWidget;
}

class EnvironmentFoldersWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EnvironmentFoldersWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EnvironmentFoldersWidget();
signals:
    void shouldQuitApp();
private:
    Ui::EnvironmentFoldersWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_btnOpenConfigFolderInBrowser_clicked();
    void on_btnResetDefault_clicked();

    // SettingsWidget interface
    void on_btnOpenIconSetFolderInFileBrowser_clicked();

    void on_btnOpenThemeFolderInFileBrowser_clicked();

protected:
    void updateIcons(const QSize &size) override;
};

#endif // ENVIRONMENTFOLDERSWIDGET_H
