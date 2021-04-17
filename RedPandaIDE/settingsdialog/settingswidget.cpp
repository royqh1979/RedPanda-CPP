#include "settingswidget.h"

#include <QAbstractItemView>
#include <QCheckBox>
#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QPlainTextEdit>

SettingsWidget::SettingsWidget(const QString &name, const QString &group, QWidget *parent):
    QWidget(parent),
    mName(name),
    mGroup(group)
{

}

void SettingsWidget::init()
{
    load();
    connectInputs();
}

void SettingsWidget::load()
{
    doLoad();
    mSettingsChanged = false;
}

void SettingsWidget::save()
{
    doSave();
    mSettingsChanged = false;
}

void SettingsWidget::connectInputs()
{
    for (QLineEdit* p:findChildren<QLineEdit*>()) {
        connect(p, &QLineEdit::textChanged, this, &SettingsWidget::setSettingsChanged);
    }
    for (QCheckBox* p:findChildren<QCheckBox*>()) {
        connect(p, &QCheckBox::stateChanged, this, &SettingsWidget::setSettingsChanged);
    }
    for (QPlainTextEdit* p:findChildren<QPlainTextEdit*>()) {
        connect(p, &QPlainTextEdit::textChanged, this, &SettingsWidget::setSettingsChanged);
    }
    for (QComboBox* p: findChildren<QComboBox*>()) {
        connect(p, QOverload<int>::of(&QComboBox::currentIndexChanged) ,this, &SettingsWidget::setSettingsChanged);
    }
    for (QAbstractItemView* p: findChildren<QAbstractItemView*>()) {
        connect(p, &QAbstractItemView::activated,this, &SettingsWidget::setSettingsChanged);
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
