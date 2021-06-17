#include "editorcolorschemewidget.h"
#include "ui_editorcolorschemewidget.h"

EditorColorSchemeWidget::EditorColorSchemeWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorColorSchemeWidget)
{
    ui->setupUi(this);
}

EditorColorSchemeWidget::~EditorColorSchemeWidget()
{
    delete ui;
}
