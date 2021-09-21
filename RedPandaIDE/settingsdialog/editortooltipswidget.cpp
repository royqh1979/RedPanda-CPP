#include "editortooltipswidget.h"
#include "ui_editortooltipswidget.h"
#include "../settings.h"

EditorTooltipsWidget::EditorTooltipsWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorTooltipsWidget)
{
    ui->setupUi(this);
}

EditorTooltipsWidget::~EditorTooltipsWidget()
{
    delete ui;
}

void EditorTooltipsWidget::doLoad()
{
    ui->chkShowFunctionTips->setChecked(pSettings->editor().showFunctionTips());
    ui->grpEnableTooltips->setChecked(pSettings->editor().enableTooltips());
    ui->chkIssueTooltips->setChecked(pSettings->editor().enableIssueToolTips());
    ui->chkIdentifierTooltips->setChecked(pSettings->editor().enableIdentifierToolTips());
    ui->chkHeaderTooltips->setChecked(pSettings->editor().enableHeaderToolTips());
    ui->chkDebugTooltips->setChecked(pSettings->editor().enableDebugTooltips());

}

void EditorTooltipsWidget::doSave()
{
    pSettings->editor().setShowFunctionTips(ui->chkShowFunctionTips->isChecked());
    pSettings->editor().setEnableTooltips(ui->grpEnableTooltips->isChecked());
    pSettings->editor().setEnableIssueToolTips(ui->chkIssueTooltips->isChecked());
    pSettings->editor().setEnableIdentifierToolTips(ui->chkIdentifierTooltips->isChecked());
    pSettings->editor().setEnableHeaderToolTips(ui->chkHeaderTooltips->isChecked());
    pSettings->editor().setEnableDebugTooltips(ui->chkDebugTooltips->isChecked());
    pSettings->editor().save();
}
