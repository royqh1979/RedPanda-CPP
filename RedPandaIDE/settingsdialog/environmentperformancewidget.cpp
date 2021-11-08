#include "environmentperformancewidget.h"
#include "ui_environmentperformancewidget.h"
#include "../settings.h"

EnvironmentPerformanceWidget::EnvironmentPerformanceWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EnvironmentPerformanceWidget)
{
    ui->setupUi(this);
}

EnvironmentPerformanceWidget::~EnvironmentPerformanceWidget()
{
    delete ui;
}

void EnvironmentPerformanceWidget::doLoad()
{
    ui->chkClearWhenEditorHidden->setChecked(pSettings->codeCompletion().clearWhenEditorHidden());
}

void EnvironmentPerformanceWidget::doSave()
{
    pSettings->codeCompletion().setClearWhenEditorHidden(ui->chkClearWhenEditorHidden->isChecked());

    pSettings->codeCompletion().save();
}
