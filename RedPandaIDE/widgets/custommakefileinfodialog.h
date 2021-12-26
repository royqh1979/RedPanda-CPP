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
#ifndef CUSTOMMAKEFILEINFODIALOG_H
#define CUSTOMMAKEFILEINFODIALOG_H

#include <QDialog>

namespace Ui {
class CustomMakefileInfoDialog;
}

class CustomMakefileInfoDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CustomMakefileInfoDialog(QWidget *parent = nullptr);
    ~CustomMakefileInfoDialog();

private slots:
    void on_pushButton_clicked();

private:
    Ui::CustomMakefileInfoDialog *ui;
};

#endif // CUSTOMMAKEFILEINFODIALOG_H
