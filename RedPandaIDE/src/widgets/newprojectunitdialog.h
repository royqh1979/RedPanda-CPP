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
#ifndef NEWPROJECTUNITDIALOG_H
#define NEWPROJECTUNITDIALOG_H

#include <QDialog>

namespace Ui {
class NewProjectUnitDialog;
}

class NewProjectUnitDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewProjectUnitDialog(QWidget *parent = nullptr);
    ~NewProjectUnitDialog();

    QString folder() const;
    void setFolder(const QString& folderName);

    QString filename() const;
    void setFilename(const QString& filename);

    const QString &suffix() const;
    void setSuffix(const QString &newSuffix);

private slots:
    void onUpdateIcons();
    void on_btnBrowse_clicked();

    void on_btnOk_clicked();

    void on_btnCancel_clicked();

    void on_txtFilename_textChanged(const QString &arg1);
    void on_txtFolder_textChanged(const QString &arg1);

private:
    void updateBtnOkStatus();
private:
    Ui::NewProjectUnitDialog *ui;
private:
    QString mSuffix;

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
};

#endif // NEWPROJECTUNITDIALOG_H
