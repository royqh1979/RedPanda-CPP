#include "compileargumentswidget.h"
#include "ui_compileargumentswidget.h"

#include <QCheckBox>
#include <QComboBox>
#include <QLabel>

CompileArgumentsWidget::CompileArgumentsWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::CompileArgumentsWidget)
{
    ui->setupUi(this);
}

CompileArgumentsWidget::~CompileArgumentsWidget()
{
    delete ui;
}

QMap<QString, QString> CompileArgumentsWidget::options() const
{
    QMap
    QTabWidget* pTab = ui->tabArguments;
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
                if (pOption->choices.isEmpty()) {
                    QCheckBox* pCheckbox = static_cast<QCheckBox *>(pLayout->itemAtPosition(j,1)->widget());
                    if (pCheckbox->isChecked()) {

                        pSet->setCompileOption(key,"");
                    } else {
                        pSet->unsetCompileOption(key);
                    }
                } else {
                    QComboBox* pCombo = static_cast<QComboBox *>(pLayout->itemAtPosition(j,2)->widget());
                    if (!pCombo->currentData().toString().isEmpty()) {
                        pSet->setCompileOption(key,pCombo->currentData().toString());
                    } else {
                        pSet->unsetCompileOption(key);
                    }
                }
            }
        }
    }
}

void CompileArgumentsWidget::resetUI(Settings::PCompilerSet pSet, const QMap<QString,QString>& options)
{
    QTabWidget* pTab = ui->tabArguments;
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

    foreach (PCompilerOption pOption, CompilerInfoManager::getCompilerOptions(mCompilerType).values()) {
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
        if (pOption->choices.isEmpty()) {
            QCheckBox* pCheckbox = new QCheckBox(pWidget);
            pCheckbox->setText(pOption->name);
            pCheckbox->setChecked(options.value(pOption->key,"")==COMPILER_OPTION_ON);
            pLayout->addWidget(pCheckbox,row,1);
        } else {
            pLayout->addWidget(new QLabel(pOption->name,pWidget),row,1);
            QComboBox* pCombo = new QComboBox(pWidget);
            pCombo->addItem("","");
            for (int i=0;i<pOption->choices.length();i++) {
                const QPair<QString,QString> &choice = pOption->choices[i];
                pCombo->addItem(choice.first,choice.second);
                if (options.value(pOption->key,"") == choice.second)
                    pCombo->setCurrentIndex(i);
            }
            pLayout->addWidget(pCombo,row,2);
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
