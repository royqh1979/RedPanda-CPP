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
#ifndef COMPILERSETDIRECTORIESWIDGET_H
#define COMPILERSETDIRECTORIESWIDGET_H

#include <QWidget>
#include <QStringListModel>

namespace Ui {
class CompilerSetDirectoriesWidget;
}

class QItemSelection;

class CompilerSetDirectoriesWidget : public QWidget
{
    Q_OBJECT
    class ListModel: public QStringListModel {
    public:
       //~ListModel();
       Qt::ItemFlags flags(const QModelIndex &index) const;
    };

public:
    explicit CompilerSetDirectoriesWidget(QWidget *parent = nullptr);
    ~CompilerSetDirectoriesWidget();

    void setDirList(const QStringList& list);
    QStringList dirList() const;

private slots:
    void on_btnDelete_pressed();

    void on_btnAdd_pressed();

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);

    void on_btnRemoveInvalid_pressed();

    void onUpdateIcons();

private:
    Ui::CompilerSetDirectoriesWidget *ui;
    ListModel mModel;
};

#endif // COMPILERSETDIRECTORIESWIDGET_H
