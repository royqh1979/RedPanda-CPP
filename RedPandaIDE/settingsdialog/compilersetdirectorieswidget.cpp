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
#include "compilersetdirectorieswidget.h"
#include "ui_compilersetdirectorieswidget.h"
#include "../iconsmanager.h"

#include <QFileDialog>
#include <QStringListModel>
#include <QDebug>

CompilerSetDirectoriesWidget::CompilerSetDirectoriesWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CompilerSetDirectoriesWidget)
{
    ui->setupUi(this);

    QItemSelectionModel *m=ui->listView->selectionModel();
    ui->listView->setModel(&mModel);
    delete m;
    connect(ui->listView->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &CompilerSetDirectoriesWidget::selectionChanged);
    ui->listView->setSelectionMode(QAbstractItemView::SingleSelection);
    onUpdateIcons();
}

CompilerSetDirectoriesWidget::~CompilerSetDirectoriesWidget()
{
    delete ui;
    //qDebug()<<"compiler set directory widget deleted";
}

void CompilerSetDirectoriesWidget::setDirList(const QStringList &list)
{
    mModel.setStringList(list);
    QModelIndexList lst =ui->listView->selectionModel()->selectedIndexes();
    ui->btnDelete->setEnabled(lst.count()>0);
}

QStringList CompilerSetDirectoriesWidget::dirList() const
{
    return mModel.stringList();
}

//CompilerSetDirectoriesWidget::ListModel::~ListModel()
//{
//    qDebug()<<"compiler set directory widget list model deleted";
//}

Qt::ItemFlags CompilerSetDirectoriesWidget::ListModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags flags = Qt::NoItemFlags;
    if (index.isValid()) {
        flags = Qt::ItemIsEnabled | Qt::ItemIsDragEnabled | Qt::ItemIsEditable | Qt::ItemIsSelectable ;
    } else if (index.row() == -1) {
        // -1 means it's a drop target?
        flags = Qt::ItemIsDropEnabled;
    }
    return flags;
}

void CompilerSetDirectoriesWidget::on_btnAdd_pressed()
{
    QString folder = QFileDialog::getExistingDirectory(this,tr("Choose Folder"));
    if (!folder.isEmpty()) {
        int row = mModel.rowCount();
        mModel.insertRow(row);
        QModelIndex index= mModel.index(row,0);
        mModel.setData(index,folder,Qt::DisplayRole);
    }
}

void CompilerSetDirectoriesWidget::selectionChanged(const QItemSelection &selected, const QItemSelection &/*deselected*/)
{
    ui->btnDelete->setEnabled(!selected.isEmpty());
}

void CompilerSetDirectoriesWidget::on_btnDelete_pressed()
{
    QModelIndexList lst =ui->listView->selectionModel()->selectedIndexes();
    if (lst.count()>0) {
        mModel.removeRow(lst[0].row());
    }
}


void CompilerSetDirectoriesWidget::on_btnRemoveInvalid_pressed()
{
    QStringList lst;
    for (const QString& folder : dirList() ) {
        QFileInfo info(folder);
        if (info.exists() && info.isDir() ) {
            lst.append(folder.trimmed());
        }
    }
    setDirList(lst);
}

void CompilerSetDirectoriesWidget::onUpdateIcons()
{
    pIconsManager->setIcon(ui->btnAdd,IconsManager::ACTION_MISC_ADD);
    pIconsManager->setIcon(ui->btnDelete, IconsManager::ACTION_MISC_REMOVE);
    pIconsManager->setIcon(ui->btnRemoveInvalid, IconsManager::ACTION_MISC_VALIDATE);
}
