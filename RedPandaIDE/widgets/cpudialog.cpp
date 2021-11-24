#include "cpudialog.h"
#include "ui_cpudialog.h"
#include "../HighlighterManager.h"
#include "../mainwindow.h"
#include "../debugger.h"
#include "../settings.h"
#include "../colorscheme.h"

CPUDialog::CPUDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CPUDialog)
{
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    ui->setupUi(this);
    ui->txtCode->setHighlighter(highlighterManager.getCppHighlighter());
    ui->txtCode->setReadOnly(true);
    ui->txtCode->gutter().setShowLineNumbers(false);
    ui->txtCode->setCaretUseTextColor(true);

    ui->txtCode->codeFolding().indentGuides = false;
    ui->txtCode->codeFolding().fillIndents = false;
    ui->txtCode->setGutterWidth(0);
    ui->txtCode->setUseCodeFolding(false);
    highlighterManager.applyColorScheme(ui->txtCode->highlighter(),
                                        pSettings->editor().colorScheme());
    PColorSchemeItem item = pColorManager->getItem(pSettings->editor().colorScheme(),COLOR_SCHEME_ACTIVE_LINE);
    if (item) {
        ui->txtCode->setActiveLineColor(item->background());
    }
    item = pColorManager->getItem(pSettings->editor().colorScheme(),COLOR_SCHEME_TEXT);
    if (item) {
        ui->txtCode->setForegroundColor(item->foreground());
        ui->txtCode->setBackgroundColor(item->background());
    } else {
        ui->txtCode->setForegroundColor(palette().color(QPalette::Text));
        ui->txtCode->setBackgroundColor(palette().color(QPalette::Base));
    }
    ui->lstRegister->setModel(pMainWindow->debugger()->registerModel());

    ui->rdIntel->setChecked(pSettings->debugger().useIntelStyle());
    ui->chkBlendMode->setChecked(pSettings->debugger().blendMode());
//    RadioATT.Checked := devData.UseATTSyntax;
//    RadioIntel.Checked := not devData.UseATTSyntax;

//    fRegisters := TList.Create;
//    fAssembler := TStringList.Create;
    //updateInfo();
}

CPUDialog::~CPUDialog()
{
    delete ui;
}

void CPUDialog::updateInfo()
{
    if (pMainWindow->debugger()->executing()) {
        // Load the registers..
        sendSyntaxCommand();
        pMainWindow->debugger()->sendCommand("info", "registers");
        if (ui->chkBlendMode->isChecked())
            pMainWindow->debugger()->sendCommand("disas", "/s");
        else
            pMainWindow->debugger()->sendCommand("disas", "");
    }
}

void CPUDialog::setDisassembly(const QStringList &lines)
{
    if (lines.size()>0) {
        ui->txtFunctionName->setText(lines[0]);
    }
    int activeLine = -1;
    ui->txtCode->lines()->clear();
    for (int i=1;i<lines.size();i++) {
        QString line = lines[i];
        if (line.startsWith("=>")) {
            activeLine = i;
        }
        ui->txtCode->lines()->add(line);
    }
    if (activeLine!=-1)
        ui->txtCode->setCaretXY(BufferCoord{1,activeLine});
}

void CPUDialog::sendSyntaxCommand()
{
    // Set disassembly flavor
    if (ui->rdIntel->isChecked()) {
        pMainWindow->debugger()->sendCommand("-gdb-set", "disassembly-flavor intel");
    } else {
        pMainWindow->debugger()->sendCommand("-gdb-set", "disassembly-flavor att");
    }
}

void CPUDialog::closeEvent(QCloseEvent *event)
{
    QDialog::closeEvent(event);
    emit closed();
}

void CPUDialog::on_rdIntel_toggled(bool)
{
    sendSyntaxCommand();
    pSettings->debugger().setUseIntelStyle(ui->rdIntel->isChecked());
    pSettings->debugger().save();
}

void CPUDialog::on_rdATT_toggled(bool)
{
    sendSyntaxCommand();
    pSettings->debugger().setUseIntelStyle(ui->rdIntel->isChecked());
    pSettings->debugger().save();
}

void CPUDialog::on_chkBlendMode_stateChanged(int)
{
    updateInfo();
    pSettings->debugger().setBlendMode(ui->chkBlendMode->isCheckable());
    pSettings->debugger().save();
}
