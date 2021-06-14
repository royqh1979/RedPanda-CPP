#include "editorcolorschemewidget.h"
#include "ui_editorcolorschemewidget.h"

EditorColorSchemeWidget::EditorColorSchemeWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EditorColorSchemeWidget)
{
    ui->setupUi(this);
}

EditorColorSchemeWidget::~EditorColorSchemeWidget()
{
    delete ui;
}
