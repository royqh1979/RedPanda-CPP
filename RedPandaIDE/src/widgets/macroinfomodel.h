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
#ifndef MACROINFOMODEL_H
#define MACROINFOMODEL_H

#include <QAbstractListModel>
#include <memory>

struct MacroInfo {
    QString macro;
    QString description;
};

using PMacroInfo = std::shared_ptr<MacroInfo>;

class MacroInfoModel: public QAbstractListModel{
    Q_OBJECT
public:
    explicit MacroInfoModel(QObject* parent = nullptr);

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    PMacroInfo getInfo(const QModelIndex& index) const;
private:
    void addMacroInfo(const QString& macro, const QString& description);
private:
    QList<PMacroInfo> mMacroInfos;
};

#endif // MACROINFOMODEL_H
