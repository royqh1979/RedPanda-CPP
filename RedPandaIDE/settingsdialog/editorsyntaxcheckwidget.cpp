#include "editorsyntaxcheckwidget.h"
#include "ui_editorsyntaxcheckwidget.h"
#include "../settings.h"

EditorSyntaxCheckWidget::EditorSyntaxCheckWidget(const QString &name, const QString &group, QWidget *parent) :
    SettingsWidget(name,group,parent),
    ui(new Ui::EditorSyntaxCheckWidget)
{
    ui->setupUi(this);
}

EditorSyntaxCheckWidget::~EditorSyntaxCheckWidget()
{
    delete ui;
}

void EditorSyntaxCheckWidget::doLoad()
{
    //Auto Syntax Check
    ui->grpEnableAutoSyntaxCheck->setChecked(pSettings->editor().syntaxCheck());
    ui->chkSyntaxCheckWhenSave->setChecked(pSettings->editor().syntaxCheckWhenSave());
    ui->chkSyntaxCheckWhenLineChanged->setChecked(pSettings->editor().syntaxCheckWhenLineChanged());
}

void EditorSyntaxCheckWidget::doSave()
{
    //Auto Syntax Check
    pSettings->editor().setSyntaxCheck(ui->grpEnableAutoSyntaxCheck->isChecked());
    pSettings->editor().setSyntaxCheckWhenSave(ui->chkSyntaxCheckWhenSave->isChecked());
    pSettings->editor().setSyntaxCheckWhenLineChanged(ui->chkSyntaxCheckWhenLineChanged->isChecked());

    pSettings->editor().save();
}
