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
#include <qsynedit/document.h>
#include "cpudialog.h"
#include "ui_cpudialog.h"
#include "../syntaxermanager.h"
#include "../mainwindow.h"
#include "../debugger/debugger.h"
#include "../settings.h"
#include "../colorscheme.h"
#include "../iconsmanager.h"

CPUDialog::CPUDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CPUDialog),
    mInited(false),
    mSetting(false)
{
    setWindowFlags(windowFlags() | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint);
    setWindowFlag(Qt::WindowContextHelpButtonHint,false);
    ui->setupUi(this);
    updateSyntaxer();
    ui->txtCode->setReadOnly(true);
    ui->txtCode->gutter().setShowLineNumbers(false);
    ui->txtCode->setCaretUseTextColor(true);

    ui->txtCode->codeFolding().indentGuides = false;
    ui->txtCode->codeFolding().fillIndents = false;
    ui->txtCode->setGutterWidth(0);
    ui->txtCode->setUseCodeFolding(false);
    ui->txtCode->setRightEdge(0);
    QSynedit::EditorOptions options=ui->txtCode->getOptions();
    options.setFlag(QSynedit::EditorOption::ScrollPastEof,false);
    options.setFlag(QSynedit::EditorOption::ScrollPastEol,false);
    options.setFlag(QSynedit::EditorOption::ShowRainbowColor, false);
    ui->txtCode->setOptions(options);
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
    resetEditorFont(screenDPI());
    QItemSelectionModel *m=ui->lstRegister->selectionModel();
    ui->lstRegister->setModel(pMainWindow->debugger()->registerModel().get());
    delete m;

    ui->rdIntel->setChecked(pSettings->debugger().useIntelStyle());
    if (!ui->rdIntel->isChecked())
        ui->rdATT->setChecked(true);
    ui->chkBlendMode->setChecked(pSettings->debugger().blendMode());
    resize(pSettings->ui().CPUDialogWidth(),pSettings->ui().CPUDialogHeight());

    onUpdateIcons();
    connect(pIconsManager,&IconsManager::actionIconsUpdated,
            this, &CPUDialog::onUpdateIcons);
}

CPUDialog::~CPUDialog()
{
    delete ui;
}

void CPUDialog::updateInfo()
{
    if (pMainWindow->debugger()->executing()) {
        pMainWindow->debugger()->refreshFrame();
        // Load the registers..
        sendSyntaxCommand();
        pMainWindow->debugger()->refreshRegisters();
        pMainWindow->debugger()->disassembleCurrentFrame(ui->chkBlendMode->isChecked());
    }
}

void CPUDialog::updateButtonStates(bool enable)
{
    ui->btnStepIntoInstruction->setEnabled(enable);
    ui->btnStepOverInstruction->setEnabled(enable);
}

void CPUDialog::updateDPI(float dpi)
{
    QFont font(pSettings->environment().interfaceFont());
    font.setPixelSize(pointToPixel(pSettings->environment().interfaceFontSize(),dpi));
    font.setStyleStrategy(QFont::PreferAntialias);
    setFont(font);
    for (QWidget* p:findChildren<QWidget*>()) {
        if (p!=ui->txtCode)
            p->setFont(font);
    }
    resetEditorFont(dpi);
    onUpdateIcons();
}

void CPUDialog::setDisassembly(const QString& file, const QString& funcName,const QStringList& lines,const QList<PTrace>& traces)
{
    mSetting=true;
    ui->cbCallStack->clear();
    int currentIndex=-1;
    for (int i=0;i<traces.count();i++) {
        ui->cbCallStack->addItem(QString("%1:%2").arg(traces[i]->filename, traces[i]->funcname));
        if (file==traces[i]->filename && funcName == traces[i]->funcname)
            currentIndex=i;
    }
    ui->cbCallStack->setCurrentIndex(currentIndex);
    int activeLine = -1;
    for (int i=0;i<lines.size();i++) {
        QString line = lines[i];
        if (line.startsWith("=>") || line.startsWith("->")) {
            activeLine = i;
        }
    }
    ui->txtCode->document()->setContents(lines);
    ui->txtCode->reparseDocument();
    ui->txtCode->invalidate();
    if (activeLine!=-1)
        ui->txtCode->setCaretXYCentered(QSynedit::BufferCoord{1,activeLine+1});
    mSetting=false;
}

void CPUDialog::resetEditorFont(float dpi)
{
    QSynedit::EditorOptions options=ui->txtCode->getOptions();
    options.setFlag(QSynedit::EditorOption::LigatureSupport, pSettings->editor().enableLigaturesSupport());
    options.setFlag(QSynedit::EditorOption::ForceMonospace,
                    pSettings->editor().forceFixedFontWidth());
    ui->txtCode->setOptions(options);
    QFont f=QFont();
    f.setFamily(pSettings->editor().fontName());
    f.setFamilies(pSettings->editor().fontFamiliesWithControlFont());
    f.setPixelSize(pointToPixel(pSettings->editor().fontSize(),dpi));
    f.setStyleStrategy(QFont::PreferAntialias);
    ui->txtCode->setFont(f);
}

void CPUDialog::sendSyntaxCommand()
{
    pMainWindow->debugger()->setDisassemblyLanguage(ui->rdIntel->isChecked());
}

void CPUDialog::updateSyntaxer()
{
    if (pSettings->debugger().blendMode() && pMainWindow->debugger()->supportDisassemlyBlendMode()) {
        if (pSettings->debugger().useIntelStyle())
            ui->txtCode->setSyntaxer(syntaxerManager.getSyntaxer(QSynedit::ProgrammingLanguage::MixedAssembly));
        else
            ui->txtCode->setSyntaxer(syntaxerManager.getSyntaxer(QSynedit::ProgrammingLanguage::MixedATTAssembly));
    } else {
        if (pSettings->debugger().useIntelStyle())
            ui->txtCode->setSyntaxer(syntaxerManager.getSyntaxer(QSynedit::ProgrammingLanguage::Assembly));
        else
            ui->txtCode->setSyntaxer(syntaxerManager.getSyntaxer(QSynedit::ProgrammingLanguage::ATTAssembly));
    }
    syntaxerManager.applyColorScheme(ui->txtCode->syntaxer(),
                                        pSettings->editor().colorScheme());
}

void CPUDialog::closeEvent(QCloseEvent *event)
{
    pSettings->ui().setCPUDialogWidth(width());
    pSettings->ui().setCPUDialogHeight(height());

    QList<int> sizes = ui->splitter->sizes();
    pSettings->ui().setCPUDialogSplitterPos(sizes[0]);

    QDialog::closeEvent(event);
    emit closed();
}

void CPUDialog::on_rdIntel_toggled(bool)
{
    updateInfo();
    pSettings->debugger().setUseIntelStyle(ui->rdIntel->isChecked());
    pSettings->debugger().save();
    updateSyntaxer();
}

void CPUDialog::on_rdATT_toggled(bool)
{
    updateInfo();
    pSettings->debugger().setUseIntelStyle(ui->rdIntel->isChecked());
    pSettings->debugger().save();
    updateSyntaxer();
}

void CPUDialog::on_chkBlendMode_stateChanged(int)
{
    updateInfo();
    pSettings->debugger().setBlendMode(ui->chkBlendMode->isCheckable());
    pSettings->debugger().save();
    updateSyntaxer();
}

void CPUDialog::on_btnStepOverInstruction_clicked()
{
    pMainWindow->debugger()->stepOverInstruction();
}


void CPUDialog::on_btnStepIntoInstruction_clicked()
{
    pMainWindow->debugger()->stepIntoInstruction();
}

void CPUDialog::onUpdateIcons()
{
    pIconsManager->setIcon(ui->btnStepIntoInstruction, IconsManager::ACTION_RUN_STEP_INTO_INSTRUCTION);
    pIconsManager->setIcon(ui->btnStepOverInstruction, IconsManager::ACTION_RUN_STEP_OVER_INSTRUCTION);
}

void CPUDialog::showEvent(QShowEvent *event)
{
    QDialog::showEvent(event);
    if (!mInited) {
        mInited=true;

        QList<int> sizes = ui->splitter->sizes();
        int tabWidth = pSettings->ui().CPUDialogSplitterPos();
        int totalSize = sizes[0] + sizes[1];
        sizes[0] = tabWidth;
        sizes[1] = std::max(0,totalSize - sizes[0]);
        ui->splitter->setSizes(sizes);
    }
    ui->chkBlendMode->setVisible(pMainWindow->debugger()->supportDisassemlyBlendMode());
}


void CPUDialog::on_cbCallStack_currentIndexChanged(int index)
{
    if (mSetting)
        return ;
    pMainWindow->switchCurrentStackTrace(index);
}

