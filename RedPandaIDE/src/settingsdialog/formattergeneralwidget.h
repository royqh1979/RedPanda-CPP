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
#ifndef FORMATTERGENERALWIDGET_H
#define FORMATTERGENERALWIDGET_H

#include <QAbstractListModel>
#include <QWidget>
#include "settingswidget.h"
#include "../utils.h"
#include "../settings.h"

namespace Ui {
class FormatterGeneralWidget;
}

struct FormatterStyleItem {
    QString name;
    QString description;
    FormatterBraceStyle style;
    explicit FormatterStyleItem(const QString& name,
                                const QString& description,
                                FormatterBraceStyle style);
};
using PFormatterStyleItem = std::shared_ptr<FormatterStyleItem>;

class FormatterStyleModel : public QAbstractListModel{
    Q_OBJECT
    // QAbstractItemModel interface
public:
    explicit FormatterStyleModel(QObject *parent=nullptr);
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    PFormatterStyleItem getStyle(const QModelIndex &index);
    PFormatterStyleItem getStyle(int index);
private:
    QList<PFormatterStyleItem> mStyles;
};


class FormatterGeneralWidget : public SettingsWidget
{
    Q_OBJECT

public:
    explicit FormatterGeneralWidget(const QString& name, const QString& group, QWidget *parent = nullptr);
    ~FormatterGeneralWidget();
private slots:
    void onBraceStyleChanged();

    void on_chkBreakMaxCodeLength_stateChanged(int arg1);

    void updateDemo();
private:
    void updateCodeFormatter(Settings::CodeFormatter& format);

private:
    Ui::FormatterGeneralWidget *ui;
    FormatterStyleModel mStylesModel;

    // SettingsWidget interface
protected:
    void doLoad() override;
    void doSave() override;

    // SettingsWidget interface
};

#endif // FORMATTERGENERALWIDGET_H
