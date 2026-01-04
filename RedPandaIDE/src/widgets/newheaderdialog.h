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
#ifndef NEWHEADERDIALOG_H
#define NEWHEADERDIALOG_H

#include <QDialog>

namespace Ui {
class NewHeaderDialog;
}

class NewHeaderDialog : public QDialog
{
    Q_OBJECT

public:
    explicit NewHeaderDialog(QWidget *parent = nullptr);
    ~NewHeaderDialog();
    QString headerName() const;
    QString path() const;
    void setHeaderName(const QString& name);
    void setPath(const QString& location);

private:
    Ui::NewHeaderDialog *ui;

private:
    void onUpdateIcons();

    // QWidget interface
protected:
    void closeEvent(QCloseEvent *event) override;
private slots:
    void on_btnCreate_clicked();
    void on_btnCancel_clicked();
    void on_btnBrowse_clicked();
};

#endif // NEWHEADERDIALOG_H
