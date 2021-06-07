#include "settingsdialog.h"
#include "ui_settingsdialog.h"
#include "settingswidget.h"
#include "compilersetoptionwidget.h"
#include "editorgeneralwidget.h"
#include <QDebug>
#include <QMessageBox>

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

    pEditorGeneralWidget = new EditorGeneralWidget(tr("General"),tr("Editor"));
    pEditorGeneralWidget->init();
    addWidget(pEditorGeneralWidget);
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
        saveCurrentPageSettings(true);
        SettingsWidget* pWidget = mSettingWidgets[i];
        if (ui->scrollArea->widget()!=nullptr) {
            QWidget* w = ui->scrollArea->takeWidget();
            w->setParent(nullptr);
        }
        ui->scrollArea->setWidget(pWidget);
        ui->lblWidgetCaption->setText(QString("%1 > %2").arg(pWidget->group()).arg(pWidget->name()));

        ui->btnApply->setEnabled(false);
    }
}

void SettingsDialog::widget_settings_changed(bool value)
{
    ui->btnApply->setEnabled(value);
}

void SettingsDialog::on_btnCancle_pressed()
{
    this->close();
}

void SettingsDialog::on_btnApply_pressed()
{
    saveCurrentPageSettings(false);
}

void SettingsDialog::on_btnOk_pressed()
{
    saveCurrentPageSettings(false);
    this->close();
}

void SettingsDialog::saveCurrentPageSettings(bool confirm)
{
    if (ui->scrollArea->widget()==ui->scrollAreaWidgetContents)
        return;
    SettingsWidget* pWidget = (SettingsWidget*) ui->scrollArea->widget();
    if (!pWidget->isSettingsChanged())
        return;
    if (confirm) {
        if (QMessageBox::information(this,tr("Save Changes"),
               tr("There are changes in the settings, do you want to save them before swtich to other page?"),
               QMessageBox::Yes, QMessageBox::No)!=QMessageBox::Yes) {
            return;
        }
    }
    pWidget->save();
}
