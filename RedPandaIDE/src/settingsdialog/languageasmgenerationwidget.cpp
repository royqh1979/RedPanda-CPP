#include "languageasmgenerationwidget.h"
#include "ui_languageasmgenerationwidget.h"
#include "../settings.h"

LanguageAsmGenerationWidget::LanguageAsmGenerationWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::LanguageAsmGenerationWidget)
{
    ui->setupUi(this);
#ifndef Q_OS_WIN
    ui->chkNoSEHDirectives->setText(tr("Don't generate cli directives."));
#endif
#if !defined(ARCH_X86_64) && !defined(ARCH_X86)
    ui->grpX86Syntax->setVisible(false);
#endif
}

LanguageAsmGenerationWidget::~LanguageAsmGenerationWidget()
{
    delete ui;
}

void LanguageAsmGenerationWidget::doLoad()
{
    ui->chkNoDebugDirectives->setChecked(pSettings->languages().noDebugDirectivesWhenGenerateASM());
    ui->chkNoSEHDirectives->setChecked(pSettings->languages().noSEHDirectivesWhenGenerateASM());
#if defined(ARCH_X86_64) || defined(ARCH_X86)
    switch(pSettings->languages().x86DialectOfASMGenerated()) {
    case Settings::Languages::X86ASMDialect::ATT:
        ui->rbATT->setChecked(true);
        break;
    case Settings::Languages::X86ASMDialect::Intel:
        ui->rbIntel->setChecked(true);
        break;
    }
#endif
}

void LanguageAsmGenerationWidget::doSave()
{
    pSettings->languages().setNoDebugDirectivesWhenGenerateASM(ui->chkNoDebugDirectives->isChecked());
    pSettings->languages().setNoSEHDirectivesWhenGenerateASM(ui->chkNoSEHDirectives->isChecked());
#if defined(ARCH_X86_64) || defined(ARCH_X86)
    if (ui->rbATT->isChecked()) {
        pSettings->languages().setX86DialectOfASMGenerated(Settings::Languages::X86ASMDialect::ATT);
    } else {
        pSettings->languages().setX86DialectOfASMGenerated(Settings::Languages::X86ASMDialect::Intel);
    }
#endif
    pSettings->languages().save();
}
