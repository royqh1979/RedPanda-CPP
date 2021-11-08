#include "editorcodecompletionwidget.h"
#include "ui_editorcodecompletionwidget.h"
#include "../settings.h"
#include "../mainwindow.h"
#include "../symbolusagemanager.h"

EditorCodeCompletionWidget::EditorCodeCompletionWidget(const QString& name, const QString& group,
                                                       QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorCodeCompletionWidget)
{
    ui->setupUi(this);
}

EditorCodeCompletionWidget::~EditorCodeCompletionWidget()
{
    delete ui;
}

void EditorCodeCompletionWidget::doLoad()
{
    ui->grpEnabled->setChecked(pSettings->codeCompletion().enabled());

    ui->chkParseLocalFiles->setChecked(pSettings->codeCompletion().parseLocalHeaders());
    ui->chkParseSystemFiles->setChecked(pSettings->codeCompletion().parseGlobalHeaders());

    ui->spinWidth->setValue(pSettings->codeCompletion().width());
    ui->spinHeight->setValue(pSettings->codeCompletion().height());

    ui->chkShowSuggestionWhileTyping->setChecked(pSettings->codeCompletion().showCompletionWhileInput());
    ui->chkRecordUsage->setChecked(pSettings->codeCompletion().recordUsage());
    ui->chkSortByScope->setChecked(pSettings->codeCompletion().sortByScope());
    ui->chkShowKeywords->setChecked(pSettings->codeCompletion().showKeywords());
    ui->chkIgnoreCases->setChecked(pSettings->codeCompletion().ignoreCase());
    ui->chkAppendFunc->setChecked(pSettings->codeCompletion().appendFunc());
    ui->chkShowCodeIns->setChecked(pSettings->codeCompletion().showCodeIns());
    ui->chkClearWhenEditorHidden->setChecked(pSettings->codeCompletion().clearWhenEditorHidden());
}

void EditorCodeCompletionWidget::doSave()
{
    //font
    pSettings->codeCompletion().setEnabled(ui->grpEnabled->isChecked());

    pSettings->codeCompletion().setParseLocalHeaders(ui->chkParseLocalFiles->isChecked());
    pSettings->codeCompletion().setParseGlobalHeaders(ui->chkParseSystemFiles->isChecked());

    pSettings->codeCompletion().setWidth(ui->spinWidth->value());
    pSettings->codeCompletion().setHeight(ui->spinHeight->value());

    pSettings->codeCompletion().setShowCompletionWhileInput(ui->chkShowSuggestionWhileTyping->isChecked());
    pSettings->codeCompletion().setRecordUsage(ui->chkRecordUsage->isChecked());
    pSettings->codeCompletion().setSortByScope(ui->chkSortByScope->isChecked());
    pSettings->codeCompletion().setShowKeywords(ui->chkShowKeywords->isChecked());
    pSettings->codeCompletion().setIgnoreCase(ui->chkIgnoreCases->isChecked());
    pSettings->codeCompletion().setAppendFunc(ui->chkAppendFunc->isChecked());
    pSettings->codeCompletion().setShowCodeIns(ui->chkShowCodeIns->isChecked());
    pSettings->codeCompletion().setClearWhenEditorHidden(ui->chkClearWhenEditorHidden->isChecked());

    pSettings->codeCompletion().save();
}


void EditorCodeCompletionWidget::on_btnClearUsageData_clicked()
{
    pMainWindow->symbolUsageManager()->reset();
}

