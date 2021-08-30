#include "editormiscwidget.h"
#include "ui_editormiscwidget.h"
#include "../settings.h"

EditorMiscWidget::EditorMiscWidget(const QString& name, const QString& group,
                                   QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorMiscWidget)
{
    ui->setupUi(this);
}

EditorMiscWidget::~EditorMiscWidget()
{
    delete ui;
}

void EditorMiscWidget::doLoad()
{

}

void EditorMiscWidget::doSave()
{
    pSettings->editor().save();
}
