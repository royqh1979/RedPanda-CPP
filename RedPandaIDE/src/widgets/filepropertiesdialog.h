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
#ifndef FILEPROPERTIESDIALOG_H
#define FILEPROPERTIESDIALOG_H

#include <QAbstractListModel>
#include <QDialog>

namespace Ui {
class FilePropertiesDialog;
}

class FilePropertiesModel: public QAbstractListModel {
    Q_OBJECT
public:
    explicit FilePropertiesModel(QObject* parent=nullptr);

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};
class Editor;
class FilePropertiesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit FilePropertiesDialog(Editor* activeEditor,QWidget *parent = nullptr);
    ~FilePropertiesDialog();

private:
    void calcFile(Editor* editor,
                  int& totalLines,
                  int &commentLines,
                  int &emptyLines,
                  int &codeLines,
                  int &includeLines,
                  int &charCounts);
private:
    FilePropertiesModel mModel;
    Editor * mActiveEditor;
private:
    Ui::FilePropertiesDialog *ui;

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) override;
private slots:
    void on_cbFiles_currentIndexChanged(int index);
    void on_btnOK_clicked();
};

#endif // FILEPROPERTIESDIALOG_H
