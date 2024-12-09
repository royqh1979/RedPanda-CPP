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
#ifndef ENVIRONMENTDATASIZEWIDGET_H
#define ENVIRONMENTDATASIZEWIDGET_H

#include "settings.h"
#include "settingswidget.h"
#include "utils.h"

namespace Ui {
class EnvironmentDataSizeWidget;
}

class EnvironmentDataSizeWidget : public SettingsWidget
{
    Q_OBJECT

    using DataSizeDialect = Settings::Environment::DataSizeDialect;

public:
    explicit EnvironmentDataSizeWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EnvironmentDataSizeWidget();

private:
    void updateScalePreview(DataSizeDialect dialect);

private:
    Ui::EnvironmentDataSizeWidget *ui;

private slots:
    void on_rbSiDecimal_clicked();
    void on_rbIecBinary_clicked();
    void on_rbJedecBinary_clicked();

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
    void updateIcons(const QSize &size) override;
};

#endif // ENVIRONMENTDATASIZEWIDGET_H
