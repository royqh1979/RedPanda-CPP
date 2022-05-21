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
#ifndef NEWCLASSDIALOG_H
#define NEWCLASSDIALOG_H

#include <QDialog>

namespace Ui {
class NewClassDialog;
}

class NewClassDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewClassDialog(QWidget *parent = nullptr);
    ~NewClassDialog();

    QString className() const;
    QString baseClass() const;
    QString headerName() const;
    QString sourceName() const;
    QString path() const;
    void setPath(const QString& location);

private slots:
    void on_btnCancel_clicked();

    void on_btnCreate_clicked();

    void on_btnBrowsePath_clicked();

    void on_txtClassName_textChanged(const QString &arg1);

private:
    Ui::NewClassDialog *ui;

private:
    void onUpdateIcons();

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // NEWCLASSDIALOG_H
