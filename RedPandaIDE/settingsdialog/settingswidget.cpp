#include "settingswidget.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QGroupBox>
#include <QLineEdit>
#include <QListView>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QRadioButton>
#include <QSpinBox>
#include "../widgets/coloredit.h"
#include "../utils.h"
#include "../iconsmanager.h"

SettingsWidget::SettingsWidget(const QString &name, const QString &group, QWidget *parent):
    QWidget(parent),
    mSettingsChanged(false),
    mName(name),
    mGroup(group)
{
}

void SettingsWidget::init()
{
    connect(pIconsManager,&IconsManager::actionIconsUpdated,
            this, &SettingsWidget::onUpdateIcons);
    onUpdateIcons();
    load();
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

void SettingsWidget::updateIcons(const QSize &)
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
    for (ColorEdit* p:findChildren<ColorEdit*>()) {
        connect(p, &ColorEdit::colorChanged, this, &SettingsWidget::setSettingsChanged);
    }
    for (QComboBox* p: findChildren<QComboBox*>()) {
        connect(p, QOverload<int>::of(&QComboBox::currentIndexChanged) ,this, &SettingsWidget::setSettingsChanged);
    }
    for (QAbstractItemView* p: findChildren<QAbstractItemView*>()) {
        connectAbstractItemView(p);
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
    for (QPlainTextEdit* p:findChildren<QPlainTextEdit*>()) {
        disconnect(p, &QPlainTextEdit::textChanged, this, &SettingsWidget::setSettingsChanged);
    }
    for (QComboBox* p: findChildren<QComboBox*>()) {
        disconnect(p, QOverload<int>::of(&QComboBox::currentIndexChanged) ,this, &SettingsWidget::setSettingsChanged);
    }
    for (QAbstractItemView* p: findChildren<QAbstractItemView*>()) {
        disconnectAbstractItemView(p);
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

void SettingsWidget::onUpdateIcons()
{
    updateIcons(pIconsManager->actionIconSize());
}
