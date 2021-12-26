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
#ifndef EDITORSNIPPETWIDGET_H
#define EDITORSNIPPETWIDGET_H

#include <QWidget>
#include "settingswidget.h"
#include "../codesnippetsmanager.h"

namespace Ui {
class EditorSnippetWidget;
}

class EditorSnippetWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EditorSnippetWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EditorSnippetWidget();

private:
    Ui::EditorSnippetWidget *ui;
    CodeSnippetsModel mModel;
private:
    bool mUpdatingCode;
    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
private slots:
    void on_btnRemove_clicked();
    void on_btnAdd_clicked();

    // SettingsWidget interface
protected:
    void updateIcons(const QSize &size) override;
};

#endif // EDITORSNIPPETWIDGET_H
