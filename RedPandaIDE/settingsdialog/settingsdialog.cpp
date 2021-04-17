#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "settingswidget.h"
#include "compilersetoptionwidget.h"
#include <QDebug>

SettingsDialog::SettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SettingsDialog)
{
    ui->setupUi(this);

    ui->widgetsView->setModel(&model);

    model.setHorizontalHeaderLabels(QStringList());

    ui->btnApply->setEnabled(false);

    pCompilerSetOptionWidget = new CompilerSetOptionWidget(tr("Compiler Set"),tr("Compiler"));
    pCompilerSetOptionWidget->init();

    addWidget(pCompilerSetOptionWidget);
}

SettingsDialog::~SettingsDialog()
{
    for (SettingsWidget* p:mSettingWidgets) {
        p->setParent(nullptr);
        delete p;
    }
    delete ui;
}

void SettingsDialog::addWidget(SettingsWidget *pWidget)
{
    QList<QStandardItem*> items = model.findItems(pWidget->group());
    QStandardItem* pGroupItem;
    if (items.count() == 0 ) {
        pGroupItem = new QStandardItem(pWidget->group());
        pGroupItem->setData(-1, GetWidgetIndexRole);
        model.appendRow(pGroupItem);
    } else {
        pGroupItem = items[0];
    }
    mSettingWidgets.append(pWidget);
    QStandardItem* pWidgetItem = new QStandardItem(pWidget->name());
    pWidgetItem->setData(mSettingWidgets.count()-1, GetWidgetIndexRole);
    pGroupItem->appendRow(pWidgetItem);
    connect(pWidget, &SettingsWidget::settingsChanged,
                                this , &SettingsDialog::widget_settings_changed);
}


void SettingsDialog::on_widgetsView_clicked(const QModelIndex &index)
{
    if (!index.isValid())
        return;
    int i = index.data(GetWidgetIndexRole).toInt();
    if (i>=0) {
        if (ui->scrollArea->widget()!=ui->scrollAreaWidgetContents) {
            //todo save change

        }
        SettingsWidget* pWidget = mSettingWidgets[i];
        ui->scrollArea->setWidget(pWidget);
        ui->lblWidgetCaption->setText(QString("%1 > %2").arg(pWidget->group()).arg(pWidget->name()));

        ui->btnApply->setEnabled(false);
    }
}

void SettingsDialog::widget_settings_changed(bool value)
{
    ui->btnApply->setEnabled(value);
}
