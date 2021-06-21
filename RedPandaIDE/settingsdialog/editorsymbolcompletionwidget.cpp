#include "editorsymbolcompletionwidget.h"
#include "ui_editorsymbolcompletionwidget.h"
#include "../settings.h"

EditorSymbolCompletionWidget::EditorSymbolCompletionWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorSymbolCompletionWidget)
{
    ui->setupUi(this);
}

EditorSymbolCompletionWidget::~EditorSymbolCompletionWidget()
{
    delete ui;
}

void EditorSymbolCompletionWidget::doLoad()
{
    ui->grpCompleSymbols->setChecked(pSettings->editor().completeSymbols());
    ui->chkCompleteBrace->setChecked(pSettings->editor().completeBrace());
    ui->chkCompleteBracket->setChecked(pSettings->editor().completeBracket());
    ui->chkCompleteComments->setChecked(pSettings->editor().completeComment());
    ui->chkCompleteDoubleQuotation->setChecked(pSettings->editor().completeDoubleQuote());
    ui->chkCompleteGlobalInclude->setChecked(pSettings->editor().completeGlobalInclude());
    ui->chkCompleteParenthesis->setChecked(pSettings->editor().completeParenthese());
    ui->chkCompleteSingleQuotation->setChecked(pSettings->editor().completeSingleQuote());
    ui->chkRemoveMatchingSymbols->setChecked(pSettings->editor().removeMathcingSymbol());
    ui->chkSkipMathingSymbols->setChecked(pSettings->editor().overwriteSymbols());
}

void EditorSymbolCompletionWidget::doSave()
{
    pSettings->editor().setCompleteSymbols(ui->grpCompleSymbols->isChecked());
    pSettings->editor().setCompleteBrace(ui->chkCompleteBrace->isChecked());
    pSettings->editor().setCompleteBracket(ui->chkCompleteBracket->isChecked());
    pSettings->editor().setCompleteComment(ui->chkCompleteComments->isChecked());
    pSettings->editor().setCompleteDoubleQuote(ui->chkCompleteDoubleQuotation->isChecked());
    pSettings->editor().setCompleteGlobalInclude(ui->chkCompleteGlobalInclude->isChecked());
    pSettings->editor().setCompleteParenthese(ui->chkCompleteParenthesis->isChecked());
    pSettings->editor().setCompleteSingleQuote(ui->chkCompleteSingleQuotation->isChecked());
    pSettings->editor().setRemoveMathcingSymbol(ui->chkRemoveMatchingSymbols->isChecked());
    pSettings->editor().setOverwriteSymbols(ui->chkSkipMathingSymbols->isChecked());

    pSettings->editor().save();
}
