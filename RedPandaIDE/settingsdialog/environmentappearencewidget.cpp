#include "environmentappearencewidget.h"
#include "ui_environmentappearencewidget.h"

#include <QApplication>
#include <QStyleFactory>
#include "../settings.h"
#include "../mainwindow.h"

EnvironmentAppearenceWidget::EnvironmentAppearenceWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentAppearenceWidget)
{
    ui->setupUi(this);
    ui->cbTheme->addItem("default");
    ui->cbTheme->addItem("dark");
    ui->cbTheme->addItem("dracula");
    ui->cbTheme->addItem("light");
    QStyleFactory factory;
    for (QString name:factory.keys()) {
        ui->cbTheme->addItem(name);
    }
    ui->cbLanguage->addItem("English","en");
    ui->cbLanguage->addItem(tr("Simplified Chinese"),"zh_CN");
}

EnvironmentAppearenceWidget::~EnvironmentAppearenceWidget()
{
    delete ui;
}

void EnvironmentAppearenceWidget::doLoad()
{
    ui->cbTheme->setCurrentText(pSettings->environment().theme());
    ui->cbFont->setCurrentFont(QFont(pSettings->environment().interfaceFont()));
    ui->spinFontSize->setValue(pSettings->environment().interfaceFontSize());

    for (int i=0;i<ui->cbLanguage->count();i++) {
        if (ui->cbLanguage->itemData(i) == pSettings->environment().language()) {
            ui->cbLanguage->setCurrentIndex(i);
            break;
        }
    }
}

void EnvironmentAppearenceWidget::doSave()
{
    pSettings->environment().setTheme(ui->cbTheme->currentText());
    pSettings->environment().setInterfaceFont(ui->cbFont->currentFont().family());
    pSettings->environment().setInterfaceFontSize(ui->spinFontSize->value());
    pSettings->environment().setLanguage(ui->cbLanguage->currentData().toString());

    pSettings->environment().save();
    pMainWindow->applySettings();
}
