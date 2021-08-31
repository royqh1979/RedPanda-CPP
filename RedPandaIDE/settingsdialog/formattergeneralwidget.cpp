#include "formattergeneralwidget.h"
#include "ui_formattergeneralwidget.h"
#include "../settings.h"

FormatterGeneralWidget::FormatterGeneralWidget(const QString& name, const QString& group, QWidget *parent):
    SettingsWidget(name,group,parent),
    ui(new Ui::FormatterGeneralWidget)
{
    ui->setupUi(this);
}

FormatterGeneralWidget::~FormatterGeneralWidget()
{
    delete ui;
}

void FormatterGeneralWidget::doLoad()
{
}

void FormatterGeneralWidget::doSave()
{

    pSettings->codeFormatter().save();
}
