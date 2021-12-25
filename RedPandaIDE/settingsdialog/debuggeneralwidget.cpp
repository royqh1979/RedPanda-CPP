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
    ui->grpEnableDebugConsole->setChecked(pSettings->debugger().enableDebugConsole());
    ui->chkShowDetailLog->setChecked(pSettings->debugger().showDetailLog());
    if (pSettings->debugger().useIntelStyle()) {
        ui->rbIntel->setChecked(true);
    } else {
        ui->rbATT->setChecked(true);
    }
    ui->chkShowCPUWhenSignaled->setChecked(pSettings->debugger().openCPUInfoWhenSignaled());
    ui->chkBlendMode->setChecked(pSettings->debugger().blendMode());
    ui->chkSkipSystemLib->setChecked(pSettings->debugger().skipSystemLibraries());
    ui->chkSkipProjectLib->setChecked(pSettings->debugger().skipProjectLibraries());
    ui->chkSkipCustomLib->setChecked(pSettings->debugger().skipCustomLibraries());
    ui->chkAutosaveBreakpoints->setChecked(pSettings->debugger().autosaveBreakpoints());
    ui->chkAutosaveWatches->setChecked(pSettings->debugger().autosaveWatches());
#ifdef Q_OS_WIN
    ui->grpUseGDBServer->setCheckable(true);
    ui->grpUseGDBServer->setChecked(pSettings->debugger().useGDBServer());
#endif
    ui->spinGDBServerPort->setValue(pSettings->debugger().GDBServerPort());
}

void DebugGeneralWidget::doSave()
{
    pSettings->debugger().setOnlyShowMono(ui->chkOnlyMono->isChecked());
    pSettings->debugger().setFontName(ui->cbFont->currentFont().family());
    pSettings->debugger().setFontSize(ui->sbFontSize->value());
    pSettings->debugger().setEnableDebugConsole(ui->grpEnableDebugConsole->isChecked());
    pSettings->debugger().setShowDetailLog(ui->chkShowDetailLog->isChecked());
    pSettings->debugger().setOpenCPUInfoWhenSignaled(ui->chkShowCPUWhenSignaled);
    pSettings->debugger().setUseIntelStyle(ui->rbIntel->isChecked());
    pSettings->debugger().setBlendMode(ui->chkBlendMode->isChecked());
    pSettings->debugger().setSkipSystemLibraries(ui->chkSkipSystemLib->isChecked());
    pSettings->debugger().setSkipProjectLibraries(ui->chkSkipProjectLib->isChecked());
    pSettings->debugger().setSkipCustomLibraries(ui->chkSkipCustomLib->isChecked());
    pSettings->debugger().setAutosaveBreakpoints(ui->chkAutosaveBreakpoints->isChecked());
    pSettings->debugger().setAutosaveWatches(ui->chkAutosaveWatches->isChecked());
#ifdef Q_OS_WIN
    pSettings->debugger().setUseGDBServer(ui->grpUseGDBServer->isChecked());
#endif
    pSettings->debugger().setGDBServerPort(ui->spinGDBServerPort->value());
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
