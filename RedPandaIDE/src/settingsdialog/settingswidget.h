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
#ifndef SETTINGSWIDGET_H
#define SETTINGSWIDGET_H

#include <QWidget>

class QAbstractItemView;
class SettingsWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SettingsWidget(const QString& name, const QString& group, QWidget *parent = nullptr);

    virtual void init();

    void load();
    void save();
signals:
    void settingsChanged(bool changed);
public slots:
    void onUpdateIcons();

protected:
    virtual void doLoad() = 0;
    virtual void doSave() = 0;
    virtual void onLoaded();
    void connectAbstractItemView(QAbstractItemView* pView);
    void disconnectAbstractItemView(QAbstractItemView* pView);
    virtual void updateIcons(const QSize &size) ;
public:
    const QString& group();
    const QString& name();
    bool isSettingsChanged();
    void connectInputs();
    void disconnectInputs();
public slots:
    void setSettingsChanged();
    void clearSettingsChanged();
private:

private:
    bool mSettingsChanged;
    QString mGroup;
    QString mName;

    // QWidget interface
protected:
    void showEvent(QShowEvent *event) override;
};

#endif // SETTINGSWIDGET_H
