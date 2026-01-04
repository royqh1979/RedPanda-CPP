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
#ifndef PROJECTMAKEFILEWIDGET_H
#define PROJECTMAKEFILEWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ProjectMakefileWidget;
}

class CompilerSetDirectoriesWidget;
class ProjectMakefileWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ProjectMakefileWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ProjectMakefileWidget();

private:
    Ui::ProjectMakefileWidget *ui;
    CompilerSetDirectoriesWidget * mIncludesDirWidget;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_btnBrowse_clicked();
    void on_btnInfo_clicked();

protected:
    void updateIcons(const QSize &size) override;
};

#endif // PROJECTMAKEFILEWIDGET_H
