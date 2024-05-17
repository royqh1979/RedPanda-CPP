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
#include "compileargumentswidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>

CompileArgumentsWidget::CompileArgumentsWidget(QWidget *parent) :
    QTabWidget(parent)
{

}

CompileArgumentsWidget::~CompileArgumentsWidget()
{
}

QMap<QString, QString> CompileArgumentsWidget::arguments( bool includeUnset) const
{
    QMap<QString, QString> args;
    const QTabWidget* pTab = this;
    for (int i=0;i<pTab->count();i++) {
        QString section = pTab->tabText(i);
        QWidget* pWidget = pTab->widget(i);
        QGridLayout* pLayout = static_cast<QGridLayout*>(pWidget->layout());
        if (pLayout != nullptr) {
            for (int j=1;j<pLayout->rowCount()-1;j++) {
                QString key = static_cast<QLabel *>(pLayout->itemAtPosition(j,0)->widget())->text();
                PCompilerOption pOption = CompilerInfoManager::getCompilerOption(mCompilerType,key);
                if (!pOption)
                    continue;
                switch (pOption->type) {
                case CompilerOptionType::Checkbox:
                {
                    QCheckBox* pCheckbox = static_cast<QCheckBox *>(pLayout->itemAtPosition(j,1)->widget());
                    if (pCheckbox->isChecked()) {
                        args.insert(key,COMPILER_OPTION_ON);
                    } else {
                        if (includeUnset)
                            args.insert(key,"");
                        else
                            args.remove(key);
                    }
                }
                    break;
                case CompilerOptionType::Choice:
                {
                    QComboBox* pCombo = static_cast<QComboBox *>(pLayout->itemAtPosition(j,2)->widget());
                    if (!pCombo->currentData().toString().isEmpty()) {
                        args.insert(key,pCombo->currentData().toString());
                    } else {
                        if (includeUnset)
                            args.insert(key,"");
                        else
                            args.remove(key);
                    }
                }
                    break;
                case CompilerOptionType::Input:
                {
                    QLineEdit* pText = static_cast<QLineEdit *>(pLayout->itemAtPosition(j,2)->widget());
                    QString t=pText->text().trimmed();
                    if (!t.isEmpty())
                        args.insert(key,t);
                    else {
                        if (includeUnset)
                            args.insert(key,"");
                        else
                            args.remove(key);
                    }
                }
                    break;
                case CompilerOptionType::Number:
                {
                    QSpinBox* pInput = static_cast<QSpinBox *>(pLayout->itemAtPosition(j,2)->widget());
                    int val=pInput->value();
                    if (val>=0)
                        args.insert(key,QString("%1").arg(val));
                    else {
                        if (includeUnset)
                            args.insert(key,"");
                        else
                            args.remove(key);
                    }
                }
                    break;
                }
            }
        }
    }
    return args;
}

void CompileArgumentsWidget::resetUI(Settings::PCompilerSet pSet, const QMap<QString,QString>& options)
{
    QTabWidget* pTab = this;
    while (pTab->count()>0) {
        QWidget* p=pTab->widget(0);
        if (p!=nullptr) {
            pTab->removeTab(0);
            p->setParent(nullptr);
            delete p;
        }
    }
    if (!pSet)
        return;
    mCompilerType = pSet->compilerType();

    foreach (PCompilerOption pOption, CompilerInfoManager::getCompilerOptions(mCompilerType)) {
        QWidget* pWidget = nullptr;
        for (int i=0;i<pTab->count();i++) {
            if (pOption->section == pTab->tabText(i)) {
                pWidget = pTab->widget(i);
                break;
            }
        }
        if (pWidget == nullptr) {
            pWidget = new QWidget();
            pTab->addTab(pWidget,pOption->section);
            pWidget->setLayout(new QGridLayout());
        }
        QGridLayout *pLayout = static_cast<QGridLayout*>(pWidget->layout());
        int row = pLayout->rowCount();
        QLabel* keyLabel = new QLabel(pOption->key,pWidget);
        keyLabel->setVisible(false);
        pLayout->addWidget(keyLabel,row,0);
        switch(pOption->type) {
        case CompilerOptionType::Checkbox:
        {
            QCheckBox* pCheckbox = new QCheckBox(pWidget);
            pCheckbox->setText(pOption->name);
            pCheckbox->setChecked(options.value(pOption->key,"")==COMPILER_OPTION_ON);
            pLayout->addWidget(pCheckbox,row,1,1,2);
        }
            break;
        case CompilerOptionType::Choice:
        {
            pLayout->addWidget(new QLabel(pOption->name,pWidget),row,1);
            QComboBox* pCombo = new QComboBox(pWidget);
            pCombo->addItem("","");
            for (int i=0;i<pOption->choices.length();i++) {
                const QPair<QString,QString> &choice = pOption->choices[i];
                pCombo->addItem(choice.first,choice.second);
                if (options.value(pOption->key,"") == choice.second)
                    pCombo->setCurrentIndex(i+1);
            }
            pLayout->addWidget(pCombo,row,2);
        }
            break;
        case CompilerOptionType::Input:
        {
            pLayout->addWidget(new QLabel(pOption->name,pWidget),row,1);
            QLineEdit* pInput = new QLineEdit(pWidget);
            pInput->setText(options.value(pOption->key,""));
            pLayout->addWidget(pInput,row,2);
        }
            break;
        case CompilerOptionType::Number:
        {
            pLayout->addWidget(new QLabel(pOption->name,pWidget),row,1);
            QSpinBox* pInput = new QSpinBox(pWidget);
            QString defaultValue = QString("%1").arg(pOption->defaultValue);
            bool ok;
            int val = options.value(pOption->key,defaultValue).toInt(&ok);
            if (!ok)
                val = 0;
            pInput->setSuffix(pOption->suffix);
            pInput->setMinimum(pOption->minValue);
            pInput->setMaximum(pOption->maxValue);
            pInput->setValue(val);
            pLayout->addWidget(pInput,row,2);
        }
            break;
        }
    }
    for (int i=0;i<pTab->count();i++) {
        QWidget* pWidget = pTab->widget(i);
        QGridLayout *pLayout = static_cast<QGridLayout*>(pWidget->layout());
        int row = pLayout->rowCount();
        QSpacerItem* verticalSpacer = new QSpacerItem(10, 100, QSizePolicy::Minimum, QSizePolicy::Expanding);
        pLayout->addItem(verticalSpacer,row,0);
    }
}
