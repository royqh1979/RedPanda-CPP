#include "debuggeneralwidget.h"
#include "ui_debuggeneralwidget.h"
#include "../settings.h"
#include "../mainwindow.h"

DebugGeneralWidget::DebugGeneralWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::DebugGeneralWidget)
{
    ui->setupUi(this);
}

DebugGeneralWidget::~DebugGeneralWidget()
{
    delete ui;
}

void DebugGeneralWidget::doLoad()
{
    ui->chkOnlyMono->setChecked(pSettings->debugger().onlyShowMono());
    ui->cbFont->setCurrentFont(QFont(pSettings->debugger().fontName()));
    ui->sbFontSize->setValue(pSettings->debugger().fontSize());
    ui->chkShowLog->setChecked(pSettings->debugger().showCommandLog());
    ui->chkShowFullAnnotation->setChecked(pSettings->debugger().showAnnotations());
    if (pSettings->debugger().useIntelStyle()) {
        ui->rbIntel->setChecked(true);
    } else {
        ui->rbATT->setChecked(true);
    }
    ui->chkBlendMode->setChecked(pSettings->debugger().blendMode());
}

void DebugGeneralWidget::doSave()
{
    pSettings->debugger().setOnlyShowMono(ui->chkOnlyMono->isChecked());
    pSettings->debugger().setFontName(ui->cbFont->currentFont().family());
    pSettings->debugger().setFontSize(ui->sbFontSize->value());
    pSettings->debugger().setShowCommandLog(ui->chkShowLog->isChecked());
    pSettings->debugger().setShowAnnotations(ui->chkShowFullAnnotation->isChecked());
    pSettings->debugger().setUseIntelStyle(ui->rbIntel->isChecked());
    pSettings->debugger().setBlendMode(ui->chkBlendMode->isChecked());

    pSettings->debugger().save();
    pMainWindow->updateDebuggerSettings();
}

void DebugGeneralWidget::on_chkOnlyMono_stateChanged(int)
{
    if (ui->chkOnlyMono->isChecked()) {
        ui->cbFont->setFontFilters(QFontComboBox::FontFilter::MonospacedFonts);
    } else {
        ui->cbFont->setFontFilters(QFontComboBox::FontFilter::AllFonts);
    }
}
