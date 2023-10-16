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
#ifndef ENVIRONMENTFILEASSOCIATIONWIDGET_H
#define ENVIRONMENTFILEASSOCIATIONWIDGET_H

#include <QAbstractListModel>
#include <QWidget>
#include <memory>
#include "settingswidget.h"

namespace Ui {
class EnvironmentFileAssociationWidget;
}
struct FileAssociationItem {
    QString name;
    QString suffix;
    int icon;
    bool selected;
    bool defaultSelected;
};
using PFileAssociationItem = std::shared_ptr<FileAssociationItem>;

class FileAssociationModel:public QAbstractListModel {
    Q_OBJECT
public:
    explicit FileAssociationModel(QObject* parent = nullptr);
    void addItem(const QString& name, const QString& suffix, int icon);
    void updateAssociationStates();
    void saveAssociations();
signals:
    void associationChanged();
private:
    bool checkAssociation(const QString& extension,
                          const QString& filetype,
                          const QString& verb,
                          const QString& serverApp);
    bool registerAssociation(const QString& extension,
                             const QString& filetype);
    bool unregisterAssociation(const QString& extension);
    bool unregisterFileType(const QString& fileType);
    bool registerFileType(const QString& filetype,
                            const QString& description,
                            const QString& verb,
                            const QString& serverApp,
                            int icon);
private:
    QList<PFileAssociationItem> mItems;


    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
};

class EnvironmentFileAssociationWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit EnvironmentFileAssociationWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~EnvironmentFileAssociationWidget();

private:
    Ui::EnvironmentFileAssociationWidget *ui;
    FileAssociationModel mModel;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;
};
#endif // ENVIRONMENTFILEASSOCIATIONWIDGET_H
