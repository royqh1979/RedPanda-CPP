#include "editorsymbolcompletionwidget.h"
#include "ui_editorsymbolcompletionwidget.h"

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
