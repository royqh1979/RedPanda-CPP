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
#ifndef SIGNALMESSAGEDIALOG_H
#define SIGNALMESSAGEDIALOG_H

#include <QDialog>

namespace Ui {
class SignalMessageDialog;
}

class SignalMessageDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SignalMessageDialog(QWidget *parent = nullptr);
    ~SignalMessageDialog();
    void setMessage(const QString& message);
    bool openCPUInfo();
    void setOpenCPUInfo(bool value);

private:
    Ui::SignalMessageDialog *ui;
};

#endif // SIGNALMESSAGEDIALOG_H
