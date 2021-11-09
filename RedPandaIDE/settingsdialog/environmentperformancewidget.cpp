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
#ifdef Q_OS_WIN
    MEMORYSTATUSEX statex;

    statex.dwLength = sizeof (statex);

    GlobalMemoryStatusEx (&statex);
    if (statex.ullAvailPhys < (long long int)1024*1024*1024) {
        ui->chkClearWhenEditorHidden->setEnabled(false);
        ui->chkClearWhenEditorHidden->setChecked(true);
        pSettings->codeCompletion().setClearWhenEditorHidden(true);
    }
#endif
}

void EnvironmentPerformanceWidget::doSave()
{
    pSettings->codeCompletion().setClearWhenEditorHidden(ui->chkClearWhenEditorHidden->isChecked());

    pSettings->codeCompletion().save();
}
