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
#include "settingswidget.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QListWidget>
#include <QRadioButton>
#include <QSpinBox>
#include "../widgets/coloredit.h"
#include "../utils.h"
#include "../iconsmanager.h"

SettingsWidget::SettingsWidget(const QString &name, const QString &group, QWidget *parent):
    QWidget(parent),
    mSettingsChanged(false),
    mGroup(group),
    mName(name)
{
}

void SettingsWidget::init()
{
    connect(pIconsManager,&IconsManager::actionIconsUpdated,
            this, &SettingsWidget::onUpdateIcons);
    onUpdateIcons();
    //load();
    connectInputs();
}

void SettingsWidget::load()
{
    try {
        doLoad();
        clearSettingsChanged();
    } catch (FileError & e) {
        QMessageBox::warning(nullptr,
                         tr("Load Error"),
                         e.reason());
    }
}

void SettingsWidget::save()
{
    try {
        doSave();
        clearSettingsChanged();
    } catch (FileError & e) {
        QMessageBox::warning(nullptr,
                         tr("Save Error"),
                         e.reason());
    }
}

void SettingsWidget::connectAbstractItemView(QAbstractItemView *pView)
{
    connect(pView->model(),&QAbstractItemModel::rowsInserted,this,&SettingsWidget::setSettingsChanged);
    connect(pView->model(),&QAbstractItemModel::rowsMoved,this,&SettingsWidget::setSettingsChanged);
    connect(pView->model(),&QAbstractItemModel::rowsRemoved,this,&SettingsWidget::setSettingsChanged);
    connect(pView->model(),&QAbstractItemModel::dataChanged,this,&SettingsWidget::setSettingsChanged);
    connect(pView->model(),&QAbstractItemModel::modelReset,this,&SettingsWidget::setSettingsChanged);
}

void SettingsWidget::disconnectAbstractItemView(QAbstractItemView *pView)
{
    disconnect(pView->model(),&QAbstractItemModel::rowsInserted,this,&SettingsWidget::setSettingsChanged);
    disconnect(pView->model(),&QAbstractItemModel::rowsMoved,this,&SettingsWidget::setSettingsChanged);
    disconnect(pView->model(),&QAbstractItemModel::rowsRemoved,this,&SettingsWidget::setSettingsChanged);
    disconnect(pView->model(),&QAbstractItemModel::dataChanged,this,&SettingsWidget::setSettingsChanged);
    disconnect(pView->model(),&QAbstractItemModel::modelReset,this,&SettingsWidget::setSettingsChanged);

}

void SettingsWidget::updateIcons(const QSize & /*size*/)
{

}

void SettingsWidget::connectInputs()
{
    for (QLineEdit* p:findChildren<QLineEdit*>()) {
        connect(p, &QLineEdit::textChanged, this, &SettingsWidget::setSettingsChanged);
    }
    for (QCheckBox* p:findChildren<QCheckBox*>()) {
        connect(p, &QCheckBox::toggled, this, &SettingsWidget::setSettingsChanged);
    }
    for (QRadioButton* p:findChildren<QRadioButton*>()) {
        connect(p, &QRadioButton::toggled, this, &SettingsWidget::setSettingsChanged);
    }
    for (QPlainTextEdit* p:findChildren<QPlainTextEdit*>()) {
        connect(p, &QPlainTextEdit::textChanged, this, &SettingsWidget::setSettingsChanged);
    }
    for (QSpinBox* p:findChildren<QSpinBox*>()) {
        connect(p, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsWidget::setSettingsChanged);
    }
    for (QDoubleSpinBox* p:findChildren<QDoubleSpinBox*>()) {
        connect(p, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SettingsWidget::setSettingsChanged);
    }
    for (ColorEdit* p:findChildren<ColorEdit*>()) {
        connect(p, &ColorEdit::colorChanged, this, &SettingsWidget::setSettingsChanged);
    }
    for (QComboBox* p: findChildren<QComboBox*>()) {
        connect(p, QOverload<int>::of(&QComboBox::currentIndexChanged) ,this, &SettingsWidget::setSettingsChanged);
    }
    for (QAbstractItemView* p: findChildren<QAbstractItemView*>()) {
        connectAbstractItemView(p);
    }
    for (QListWidget* p:findChildren<QListWidget*>()) {
        connect(p, QOverload<int>::of(&QListWidget::currentRowChanged) ,this, &SettingsWidget::setSettingsChanged);
    }
    for (QGroupBox* p: findChildren<QGroupBox*>()) {
        connect(p, &QGroupBox::toggled,this, &SettingsWidget::setSettingsChanged);
    }

}

void SettingsWidget::disconnectInputs()
{
    for (QLineEdit* p:findChildren<QLineEdit*>()) {
        disconnect(p, &QLineEdit::textChanged, this, &SettingsWidget::setSettingsChanged);
    }
    for (QCheckBox* p:findChildren<QCheckBox*>()) {
        disconnect(p, &QCheckBox::stateChanged, this, &SettingsWidget::setSettingsChanged);
    }
    for (QRadioButton* p:findChildren<QRadioButton*>()) {
        disconnect(p, &QRadioButton::toggled, this, &SettingsWidget::setSettingsChanged);
    }
    for (QPlainTextEdit* p:findChildren<QPlainTextEdit*>()) {
        disconnect(p, &QPlainTextEdit::textChanged, this, &SettingsWidget::setSettingsChanged);
    }
    for (QSpinBox* p:findChildren<QSpinBox*>()) {
        disconnect(p, QOverload<int>::of(&QSpinBox::valueChanged), this, &SettingsWidget::setSettingsChanged);
    }
    for (QDoubleSpinBox* p:findChildren<QDoubleSpinBox*>()) {
        disconnect(p, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SettingsWidget::setSettingsChanged);
    }

    for (ColorEdit* p:findChildren<ColorEdit*>()) {
        disconnect(p, &ColorEdit::colorChanged, this, &SettingsWidget::setSettingsChanged);
    }

    for (QComboBox* p: findChildren<QComboBox*>()) {
        disconnect(p, QOverload<int>::of(&QComboBox::currentIndexChanged) ,this, &SettingsWidget::setSettingsChanged);
    }
    for (QAbstractItemView* p: findChildren<QAbstractItemView*>()) {
        disconnectAbstractItemView(p);
    }
    for (QListWidget* p:findChildren<QListWidget*>()) {
        disconnect(p, QOverload<int>::of(&QListWidget::currentRowChanged) ,this, &SettingsWidget::setSettingsChanged);
    }
    for (QGroupBox* p: findChildren<QGroupBox*>()) {
        disconnect(p, &QGroupBox::toggled,this, &SettingsWidget::setSettingsChanged);
    }

}

const QString &SettingsWidget::group()
{
    return mGroup;
}

const QString &SettingsWidget::name()
{
    return mName;
}

bool SettingsWidget::isSettingsChanged()
{
    return mSettingsChanged;
}

void SettingsWidget::setSettingsChanged()
{
    mSettingsChanged = true;
    emit settingsChanged(true);
}

void SettingsWidget::clearSettingsChanged()
{
    mSettingsChanged = false;
    emit settingsChanged(false);
}

void SettingsWidget::showEvent(QShowEvent *event)
{
    Q_UNUSED(event);
    load();
}

void SettingsWidget::onUpdateIcons()
{
    updateIcons(pIconsManager->actionIconSize());
}
