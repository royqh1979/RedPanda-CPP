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
#ifndef PROJECTCOMPILERWIDGET_H
#define PROJECTCOMPILERWIDGET_H

#include <QMap>
#include <QWidget>
#include "settingswidget.h"

namespace Ui {
class ProjectCompilerWidget;
}

class ProjectCompilerWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit ProjectCompilerWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~ProjectCompilerWidget();
private:
    void refreshOptions();
private:
    Ui::ProjectCompilerWidget *ui;
    QMap<QString,QString> mOptions;
    bool mStaticLink;
    bool mAddCharset;
    QByteArray mExecCharset;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;

    // SettingsWidget interface
public:
    void init() override;
private slots:
    void on_cbCompilerSet_currentIndexChanged(int index);
    void on_cbEncoding_currentTextChanged(const QString &arg1);
    void on_cbEncodingDetails_currentTextChanged(const QString &arg1);
};

#endif // PROJECTCOMPILERWIDGET_H
