#include "languagecformatwidget.h"
#include "ui_languagecformatwidget.h"
#include "../settings.h"
#include "../mainwindow.h"

LanguageCFormatWidget::LanguageCFormatWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::LanguageCFormatWidget)
{
    ui->setupUi(this);
}

LanguageCFormatWidget::~LanguageCFormatWidget()
{
    delete ui;
}

void LanguageCFormatWidget::doLoad()
{
    ui->chkIndentCaseKeywords->setChecked(pSettings->languages().indentCSwitchCaseKeywords());
    ui->chkIndentClassMemberVisibilityMembers->setChecked(pSettings->languages().indentCClassMemberVisibilityKeywords());
}

void LanguageCFormatWidget::doSave()
{
    pSettings->languages().setIndentCSwitchCaseKeywords(ui->chkIndentCaseKeywords->isChecked());
    pSettings->languages().setIndentCClassMemberVisibilityKeywords(ui->chkIndentClassMemberVisibilityMembers->isChecked());
    pSettings->languages().save();
    pMainWindow->updateEditorSettings();
}
