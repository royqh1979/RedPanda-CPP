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
#ifndef CHOOSETHEMEDIALOG_H
#define CHOOSETHEMEDIALOG_H

#include <QDialog>

namespace Ui {
class ChooseThemeDialog;
}

class ChooseThemeDialog : public QDialog
{
    Q_OBJECT

public:
    enum class Theme {
        Unknown = -1,
        AutoFollowSystem = 0,
        Dark,
        Light,
    };
    enum class Language {
        C,
        CPlusPlus
    };

    explicit ChooseThemeDialog(QWidget *parent = nullptr);
    ~ChooseThemeDialog();
    Theme theme();
    Language language();

private slots:
    void on_btnOk_clicked();

private:
    Ui::ChooseThemeDialog *ui;
};

#endif // CHOOSETHEMEDIALOG_H
