#include "editorcolorschemewidget.h"
#include "ui_editorcolorschemewidget.h"
#include "../settings.h"
#include "../colorscheme.h"

EditorColorSchemeWidget::EditorColorSchemeWidget(const QString& name, const QString& group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorColorSchemeWidget)
{
    ui->setupUi(this);

    for (QString schemeName: pColorManager->getSchemes()) {
        ui->cbScheme->addItem(schemeName);
    }
}

EditorColorSchemeWidget::~EditorColorSchemeWidget()
{
    delete ui;
}

void EditorColorSchemeWidget::doLoad()
{

}

void EditorColorSchemeWidget::doSave()
{

}
