#include "languageasmgenerationwidget.h"
#include "ui_languageasmgenerationwidget.h"
#include "../settings.h"

LanguageAsmGenerationWidget::LanguageAsmGenerationWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::LanguageAsmGenerationWidget)
{
    ui->setupUi(this);
#ifndef Q_OS_WIN
    ui->chkNoSEHDirectives->setVisible(false);
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
#ifdef Q_OS_WIN
    ui->chkNoSEHDirectives->setChecked(pSettings->languages().noSEHDirectivesWhenGenerateASM());
#endif
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
#ifdef Q_OS_WIN
    pSettings->languages().setNoSEHDirectivesWhenGenerateASM(ui->chkNoSEHDirectives->isChecked());
#endif
#if defined(ARCH_X86_64) || defined(ARCH_X86)
    if (ui->rbATT->isChecked()) {
        pSettings->languages().setX86DialectOfASMGenerated(Settings::Languages::X86ASMDialect::ATT);
    } else {
        pSettings->languages().setX86DialectOfASMGenerated(Settings::Languages::X86ASMDialect::Intel);
    }
#endif
    pSettings->languages().save();
}
