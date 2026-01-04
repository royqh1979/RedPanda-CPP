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
#ifndef EDITORTOOLTIPSWIDGET_H
#define EDITORTOOLTIPSWIDGET_H

#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class EditorTooltipsWidget;
}

class EditorTooltipsWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorTooltipsWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorTooltipsWidget();

private:
    Ui::EditorTooltipsWidget *ui;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};

#endif // EDITORTOOLTIPSWIDGET_H
